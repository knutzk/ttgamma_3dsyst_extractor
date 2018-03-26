#include "util/prog_opts.hh"
#include "util/output_hists.hh"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "TFile.h"


int main(int argc, char* argv[]) {
  auto opts{getProgOptions(argc, argv)};
  const auto& base_name{opts.input_file_str};

  std::vector<std::string> eta_slice_strings;
  eta_slice_strings.emplace_back("eta0006");
  eta_slice_strings.emplace_back("eta06137");
  eta_slice_strings.emplace_back("eta137152");  // this is the crack region
  eta_slice_strings.emplace_back("eta152inf");

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

  // Fill the 1D histogram for systs.
  auto h_1D = prepare1DHist(base_name, &mc_hists);
  if (!h_1D) return 1;
  output_file->cd();

  // Fill the 3D histogram for systs.
  auto h_3D = initOutput3DHist(base_name);
  for (unsigned int pt_index = 0; pt_index < pt_slice_strings.size(); ++pt_index) {
    auto pt_slice = pt_slice_strings.at(pt_index);
    for (unsigned int eta_index = 0; eta_index < eta_slice_strings.size(); ++eta_index) {
      auto eta_slice = eta_slice_strings.at(eta_index);

      // Skip eta slice 3 which is the ecal crack region.
      if (eta_index == 2) continue;

      auto file_string = appendSliceStrings(base_name, eta_slice, pt_slice);
      std::cout << "Opening file " << file_string << std::endl;
      auto file = TFile::Open(file_string.c_str(), "READ");
      if (!file) return 1;

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

  output_file->Write();

  // Example
  auto eta_bin = h_3D->GetXaxis()->FindBin(1.20);
  auto pt_bin = h_3D->GetYaxis()->FindBin(64213);
  auto ppt_bin = h_3D->GetZaxis()->FindBin(0.78);
  auto ppt_weight = h_3D->GetBinContent(eta_bin, pt_bin, ppt_bin);
  std::cout << ppt_weight << std::endl;
  return 0;
}
