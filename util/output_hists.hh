#ifndef _OUTPUT_HISTS_HH_
#define _OUTPUT_HISTS_HH_

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "TFile.h"
#include "TH1F.h"
#include "TH3F.h"

//! Append the slice strings for eta and pt to a given basic file name, i.e.
//! insert the slice suffixes in the correct position.
std::string appendSliceStrings(const std::string& str,
                               const std::string& eta_slice_string,
                               const std::string& pt_slice_string) {
  auto pos = str.find("HFT_MVA_") + 8;
  if (pos == std::string::npos) throw std::invalid_argument("");
  std::ostringstream ss;
  ss << str.substr(0, pos) << eta_slice_string;
  ss << "_" << pt_slice_string << "_";
  ss << str.substr(pos, str.length() - pos);
  return ss.str();
}

//! Create the correct base string for histogram for a given file name. This
//! essentially cuts away some parts of the given string.
std::string createHistString(const std::string& str) {
  auto pos = str.find("ph_HFT_MVA_");
  if (pos == std::string::npos) throw std::invalid_argument("");
  auto s = str.substr(pos, str.length() - pos);

  auto getPosAfterString = [](const std::string& s, const std::string& ss) {
    auto pos = s.rfind(ss);
    if (pos != std::string::npos) return pos + ss.length();
    else return std::string::npos;
  };

  pos = getPosAfterString(s, "dilepton_ppt_");
  if (pos == std::string::npos) pos = getPosAfterString(s, "singlelepton_ppt_");
  if (pos == std::string::npos) {
    std::cerr << "Could not create histogram string ";
    std::cerr << "based on "<< str << std::endl;
    throw std::invalid_argument("");
  }
  return s.substr(0, pos);
}

//! Initialise the main output object (of class TH3F).
std::unique_ptr<TH3F> initOutput3DHist(const std::string& base_name) {
  std::string hist_name{};
  if (base_name.find("dilepton") != std::string::npos) {
    hist_name = "hist_ppt_prompt_3D";
  } else {
    hist_name = "hist_ppt_fake_3D";
  }
  float pt_bins[6] = {0, 27000, 35000, 50000, 80000, 1000000};
  float eta_bins[5] = {0, 0.6, 1.37, 1.52, 2.37};
  float ppt_bins[11] = {0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
  return std::make_unique<TH3F>(hist_name.c_str(),
                                hist_name.c_str(),
                                4, eta_bins,
                                5, pt_bins,
                                10, ppt_bins);
}

//! Get a histogram from a file. Throw an exception otherwise.
TH1F* getHistogram(TFile* file, const std::string& hist_string) {
  auto h = static_cast<TH1F*>(file->Get(hist_string.c_str()));
  if (!h) throw std::invalid_argument("Error when retrieving histogram");
  return h;
}

//! Prepare a data/MC ratio, with MC scaled to data.
std::unique_ptr<TH1F> prepareDataMCRatio(TFile* file,
                                         const std::string& hist_string,
                                         std::vector<std::string>* mc_hists) {
  // Retrieve histogram for data. If the histogram does not exist, do not catch
  // the exception, because we need the data histogram. Don't let ROOT deal with
  // the memory of this histogram, we want to do this ourselves.
  auto pointer = static_cast<TH1F*>(getHistogram(file, hist_string + "Data")->Clone());
  pointer->SetDirectory(0);
  std::unique_ptr<TH1F> h_data{pointer};

  // Add all MC histograms on top of each other. If one of them does not exist,
  // handle it gracefully and remove it from the list of histograms in
  // 'mc_hists'. If the first MC histogram (usually signal) does not exist,
  // don't catch the exception.
  TH1F* h_mc;
  for (auto h_itr = mc_hists->begin(); h_itr != mc_hists->end(); ++h_itr) {
    if (h_itr == mc_hists->begin()) {
      h_mc = getHistogram(file, hist_string + mc_hists->front());
    } else {
      try {
        h_mc->Add(getHistogram(file, hist_string + *h_itr));
      } catch (std::invalid_argument) {
        std::cerr << "Could not retrieve MC histogram for \"" << *h_itr;
        std::cerr << "\". Removing it from the list of histograms" << std::endl;
        mc_hists->erase(h_itr);
      }
    }
  }

  h_data->Scale(1./h_data->Integral());
  h_mc->Scale(1./h_mc->Integral());
  h_data->Divide(h_mc);
  return h_data;
}

//! Initialise and fill the 1D output object (of class TH1F).
std::unique_ptr<TH1F> prepare1DHist(const std::string& base_name,
                                    std::vector<std::string>* mc_hists) {
  std::string hist_name{};
  if (base_name.find("dilepton") != std::string::npos) {
    hist_name = "hist_ppt_prompt_1D";
  } else {
    hist_name = "hist_ppt_fake_1D";
  }
  float ppt_bins[11] = {0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
  auto hist = std::make_unique<TH1F>(hist_name.c_str(), hist_name.c_str(), 10, ppt_bins);

  std::cout << "Opening file " << base_name << std::endl;
  auto file = TFile::Open(base_name.c_str(), "READ");
  if (!file) return nullptr;

  // Try creating the data/MC ratio from the opened file. If anything goes
  // wrong and histograms cannot be found, exit gracefully.
  std::unique_ptr<TH1F> data_mc_ratio;
  try {
    data_mc_ratio = prepareDataMCRatio(file, createHistString(base_name), mc_hists);
  } catch (std::invalid_argument) {
    std::cerr << "Retrieving histograms failed ..." << std::endl;
    return nullptr;
  }
  for (int x = 1; x <= data_mc_ratio->GetNbinsX(); ++x) {
    hist->SetBinContent(x, data_mc_ratio->GetBinContent(x));
    hist->SetBinError(x, data_mc_ratio->GetBinError(x));
  }
  file->Close();
  return hist;
}

void fill3D(TH3F* h_3D,
            const std::string& base_name,
            const std::vector<std::string>& eta_slice_strings,
            const std::vector<std::string>& pt_slice_strings,
            std::vector<std::string>* mc_hists,
            TFile* output_file) {
  for (unsigned int pt_index = 0; pt_index < pt_slice_strings.size(); ++pt_index) {
    auto pt_slice = pt_slice_strings.at(pt_index);
    for (unsigned int eta_index = 0; eta_index < eta_slice_strings.size(); ++eta_index) {
      auto eta_slice = eta_slice_strings.at(eta_index);

      // Skip eta slice 3 which is the ecal crack region.
      if (eta_index == 2) continue;

      auto file_string = appendSliceStrings(base_name, eta_slice, pt_slice);
      std::cout << "Opening file " << file_string << std::endl;
      auto file = TFile::Open(file_string.c_str(), "READ");
      if (!file) throw;

      // Try creating the data/MC ratio from the opened file. If anything goes
      // wrong and histograms cannot be found, exit gracefully.
      std::unique_ptr<TH1F> data_mc_ratio;
      try {
        data_mc_ratio = prepareDataMCRatio(file, createHistString(file_string), mc_hists);
      } catch (std::invalid_argument) {
        std::cerr << "Retrieving histograms failed ..." << std::endl;
        throw;
      }

      // Now switch to the output file, retrieve the histogram bin contents and
      // store them in the TH3F object that will be saved to the output file.
      output_file->cd();
      for (int x = 1; x <= data_mc_ratio->GetNbinsX(); ++x) {
        const float threshold_up{2.};
        const float threshold_down{0.5};
        if (data_mc_ratio->GetBinContent(x) > threshold_up) {
          h_3D->SetBinContent(eta_index + 1, pt_index + 1, x, threshold_up);
          h_3D->SetBinError(eta_index + 1, pt_index + 1, x, 1.);
        } else if (data_mc_ratio->GetBinContent(x) < threshold_down) {
          h_3D->SetBinContent(eta_index + 1, pt_index + 1, x, threshold_down);
          h_3D->SetBinError(eta_index + 1, pt_index + 1, x, 0.5);
        } else {
          h_3D->SetBinContent(eta_index + 1, pt_index + 1, x, data_mc_ratio->GetBinContent(x));
          h_3D->SetBinError(eta_index + 1, pt_index + 1, x, data_mc_ratio->GetBinError(x));
        }
      }
      file->Close();
    }
  }
}


#endif  // _OUTPUT_HISTS_HH_
