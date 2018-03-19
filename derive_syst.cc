#include <iostream>
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
  pos = s.rfind("dilepton_ppt_") + 13;
  if (pos == std::string::npos) throw std::invalid_argument("");
  return s.substr(0, pos);
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

//! Initialise the main output object (of class TH3F).
std::unique_ptr<TH3F> initOutput3DHist(const std::string& base_name) {
  std::string hist_name{};
  if (base_name.find("dilepton") != std::string::npos) {
    hist_name = "hist_ppt_prompt_3D";
  } else {
    hist_name = "hist_ppt_fake_3D";
  }
  float eta_bins[7] = {0, 0.6, 1.0, 1.37, 1.52, 2.00, 2.37};
  float pt_bins[6] = {0, 27000, 35000, 50000, 80000, 100000};
  float ppt_bins[11] = {0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
  return std::make_unique<TH3F>(hist_name.c_str(),
                                hist_name.c_str(),
                                6, eta_bins,
                                5, pt_bins,
                                10, ppt_bins);
}


// =========================================================
// =========================================================
int main(int argc, char* argv[]) {
  // We expect one additional argument to the program, otherwise exit with
  // error. This one argument will be the base name for our sliced histograms
  // (i.e. the histogram that is not sliced).
  if (argc != 2) {
    std::cerr << "Incorrect arguments" << std::endl;
    std::cerr << "Usage: " << argv[0] << " [input file]" << std::endl;
    return 1;
  }
  const std::string base_name{argv[1]};

  std::vector<std::string> eta_slice_strings;
  eta_slice_strings.emplace_back("eta0006");
  eta_slice_strings.emplace_back("eta06100");
  eta_slice_strings.emplace_back("eta100137");
  eta_slice_strings.emplace_back("eta152200");
  eta_slice_strings.emplace_back("eta200inf");

  std::vector<std::string> pt_slice_strings;
  pt_slice_strings.emplace_back("pt0027");
  pt_slice_strings.emplace_back("pt2735");
  pt_slice_strings.emplace_back("pt3550");
  pt_slice_strings.emplace_back("pt5080");
  pt_slice_strings.emplace_back("pt80inf");

  std::vector<std::string> mc_hists;
  mc_hists.emplace_back("ttphoton");
  mc_hists.emplace_back("hadronfakes");
  mc_hists.emplace_back("electronfakes");
  mc_hists.emplace_back("Zphoton");
  mc_hists.emplace_back("Wphoton");
  mc_hists.emplace_back("Other");

  auto output_file = TFile::Open("output.root", "RECREATE");
  auto histogram = initOutput3DHist(base_name);

  for (unsigned int pt_index = 0; pt_index < pt_slice_strings.size(); ++pt_index) {
    auto pt_slice = pt_slice_strings.at(pt_index);
    for (unsigned int eta_index = 0; eta_index < eta_slice_strings.size(); ++eta_index) {
      auto eta_slice = eta_slice_strings.at(eta_index);

      // Skip eta slice 4 which is the ecal crack region.
      if (eta_index == 3) continue;

      auto file_string = appendSliceStrings(base_name, eta_slice, pt_slice);
      std::cout << "Opening file " << file_string << std::endl;
      auto file = TFile::Open(file_string.c_str(), "READ");

      // Try creating the data/MC ratio from the opened file. If anything goes
      // wrong and histograms cannot be found, exit gracefully.
      std::unique_ptr<TH1F> data_mc_ratio;
      try {
        data_mc_ratio = prepareDataMCRatio(file, createHistString(file_string), &mc_hists);
      } catch (std::invalid_argument) {
        std::cerr << "Retrieving histograms failed ..." << std::endl;
        return 1;
      }

      // Now switch to the output file, retrieve the histogram bin contents and
      // store them in the TH3F object that will be saved to the output file.
      output_file->cd();
      for (int x = 1; x <= data_mc_ratio->GetNbinsX(); ++x) {
        histogram->SetBinContent(eta_index + 1, pt_index + 1, x, data_mc_ratio->GetBinContent(x));
        histogram->SetBinError(eta_index + 1, pt_index + 1, x, data_mc_ratio->GetBinError(x));
      }
      file->Close();
    }
  }

  output_file->Write();

  // Example
  auto eta_bin = histogram->GetXaxis()->FindBin(1.20);
  auto pt_bin = histogram->GetYaxis()->FindBin(64213);
  auto ppt_bin = histogram->GetZaxis()->FindBin(0.78);
  auto ppt_weight = histogram->GetBinContent(eta_bin, pt_bin, ppt_bin);
  std::cout << ppt_weight << std::endl;
  return 0;
}
