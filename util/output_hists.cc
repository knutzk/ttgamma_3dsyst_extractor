#include "util/output_hists.hh"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "TFile.h"
#include "TH1F.h"
#include "TH3F.h"

namespace {
const float pt_bins[6] = {0, 27000, 35000, 50000, 80000, 1000000};
const float eta_bins[5] = {0, 0.6, 1.37, 1.52, 2.37};
const float ppt_bins[11] = {0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};

//! Append the slice strings for eta and pt to a given basic file
//! name, i.e. insert the slice suffixes in the correct position.
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

//! Create the correct base string for histogram for a given file
//! name. This essentially cuts away some parts of the given
//! string.
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

//! Determine name for output histograms (i.e. which systematic)
//! based on a given file path.
std::string det_output_hist_string(const std::string& file_path) {
  if (file_path.find("dilepton") != std::string::npos) {
    return "hist_ppt_prompt";
  } else {
    return "hist_ppt_fake";
  }
}

//! Get a histogram from a file. Throw an exception otherwise.
auto get_hist(TFile* file, const std::string& hist_string) {
  auto h = static_cast<TH1F*>(file->Get(hist_string.c_str()));
  if (!h) throw std::invalid_argument("histogram not found");
  return h;
}

//! Prepare a data/MC ratio, with MC scaled to data.
std::unique_ptr<TH1F> prepareDataMCRatio(TFile* file,
                                         const std::string& hist_string,
                                         std::vector<std::string>* mc_hists) {
  // Retrieve histogram for data. If the histogram does not
  // exist, do not catch the exception, because we need the data
  // histogram. Don't let ROOT deal with the memory of this
  // histogram, we want to do this ourselves.
  auto pointer = static_cast<TH1F*>(get_hist(file, hist_string + "Data")->Clone());
  pointer->SetDirectory(0);
  std::unique_ptr<TH1F> h_data{pointer};

  // Add all MC histograms on top of each other. If one of them
  // does not exist, handle it gracefully and remove it from the
  // list of histograms in 'mc_hists'. If the first MC histogram
  // (usually signal) does not exist, don't catch the exception.
  TH1F* h_mc;
  for (auto h_itr = mc_hists->begin(); h_itr != mc_hists->end(); ++h_itr) {
    if (h_itr == mc_hists->begin()) {
      h_mc = get_hist(file, hist_string + mc_hists->front());
    } else {
      try {
        h_mc->Add(get_hist(file, hist_string + *h_itr));
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
}  // namespace (anonymous)


SystHist1D::SystHist1D(const std::string& file_path)
  : SystHist1D{(det_output_hist_string(file_path) + "_1D").c_str(),
               (det_output_hist_string(file_path) + "_1D").c_str(),
               10, ppt_bins} {
  file_path_ = file_path;
}

void SystHist1D::setMCHists(std::vector<std::string>* hists) {
  mc_hists_ = hists;
}

void SystHist1D::fillFromRatios() {
  std::cout << "Opening file " << file_path_ << std::endl;
  auto file = TFile::Open(file_path_.c_str(), "READ");
  if (!file) return;

  // Try creating the data/MC ratio from the opened file. If
  // anything goes wrong and histograms cannot be found, exit
  // gracefully.
  std::unique_ptr<TH1F> data_mc_ratio;
  try {
    data_mc_ratio = prepareDataMCRatio(file, createHistString(file_path_), mc_hists_);
  } catch (std::invalid_argument) {
    std::cerr << "Retrieving histograms failed ..." << std::endl;
    return;
  }
  for (int x = 1; x <= data_mc_ratio->GetNbinsX(); ++x) {
    this->SetBinContent(x, data_mc_ratio->GetBinContent(x));
    this->SetBinError(x, data_mc_ratio->GetBinError(x));
  }
  file->Close();
}

SystHist3D::SystHist3D(const std::string& file_path)
  : SystHist3D{(det_output_hist_string(file_path) + "_3D").c_str(),
               (det_output_hist_string(file_path) + "_3D").c_str(),
               4, eta_bins,
               5, pt_bins,
               10, ppt_bins} {
  file_path_ = file_path;
  this->SetDirectory(0);
}

void SystHist3D::setMCHists(std::vector<std::string>* hists) {
  mc_hists_ = hists;
}

void SystHist3D::setEtaSlices(std::vector<std::string>* strings) {
  eta_slices_ = strings;
}

void SystHist3D::setPtSlices(std::vector<std::string>* strings) {
  pt_slices_ = strings;
}

void SystHist3D::fillFromRatios() {
  for (unsigned int pt_index = 0; pt_index < pt_slices_->size(); ++pt_index) {
    auto pt_slice = pt_slices_->at(pt_index);
    for (unsigned int eta_index = 0; eta_index < eta_slices_->size(); ++eta_index) {
      auto eta_slice = eta_slices_->at(eta_index);

      // Skip eta slice 3 which is the ecal crack region.
      if (eta_index == 2) continue;

      auto file_string = appendSliceStrings(file_path_, eta_slice, pt_slice);
      std::cout << "Opening file " << file_string << std::endl;
      auto file = TFile::Open(file_string.c_str(), "READ");
      if (!file) throw;

      // Try creating the data/MC ratio from the opened file. If
      // anything goes wrong and histograms cannot be found, exit
      // gracefully.
      std::unique_ptr<TH1F> data_mc_ratio;
      try {
        data_mc_ratio = prepareDataMCRatio(file, createHistString(file_string), mc_hists_);
      } catch (std::invalid_argument) {
        std::cerr << "Retrieving histograms failed ..." << std::endl;
        throw;
      }

      // Now switch to the output file, retrieve the histogram
      // bin contents and store them in the TH3F object that will
      // be saved to the output file.
      for (int x = 1; x <= data_mc_ratio->GetNbinsX(); ++x) {
        const float threshold_up{2.};
        const float threshold_down{0.5};
        if (data_mc_ratio->GetBinContent(x) > threshold_up) {
          this->SetBinContent(eta_index + 1, pt_index + 1, x, threshold_up);
          this->SetBinError(eta_index + 1, pt_index + 1, x, 1.);
        } else if (data_mc_ratio->GetBinContent(x) < threshold_down) {
          this->SetBinContent(eta_index + 1, pt_index + 1, x, threshold_down);
          this->SetBinError(eta_index + 1, pt_index + 1, x, 0.5);
        } else {
          this->SetBinContent(eta_index + 1, pt_index + 1, x, data_mc_ratio->GetBinContent(x));
          this->SetBinError(eta_index + 1, pt_index + 1, x, data_mc_ratio->GetBinError(x));
        }
      }
      file->Close();
    }
  }
}
