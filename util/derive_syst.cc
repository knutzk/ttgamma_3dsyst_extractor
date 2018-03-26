#include "util/prog_opts.hh"
#include "util/output_hists.hh"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "TFile.h"


int main(int argc, char* argv[]) {
  auto opts{getProgOptions(argc, argv)};
  const auto& base_name{opts.io.input};

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

  // Fill the 1D histogram for systs.
  auto h_1D = std::make_unique<SystHist1D>(base_name);
  h_1D->setMCHists(&mc_hists);
  try {
    h_1D->fillFromRatios();
  } catch (const std::invalid_argument& exc) {
    std::cerr << "Creating 1D histogram failed: " << exc.what() << std::endl;
    return 1;
  }

  // Fill the 3D histogram for systs.
  auto h_3D = std::make_unique<SystHist3D>(base_name);
  h_3D->setMCHists(&mc_hists);
  h_3D->setEtaSlices(&eta_slice_strings);
  h_3D->setPtSlices(&pt_slice_strings);
  try {
    h_3D->fillFromRatios();
  } catch (const std::invalid_argument& exc) {
    std::cerr << "Creating 3D histogram failed: " << exc.what() << std::endl;
    return 1;
  }

  // Open the output file, write the two histograms and close it.
  auto output_file = TFile::Open(opts.io.output.c_str(), "RECREATE");
  output_file->cd();
  h_1D->Write();
  h_3D->Write();
  output_file->Write();
  output_file->Close();

  // Example to determine correct bins and retrieve weights.
  auto eta_bin = h_3D->GetXaxis()->FindBin(1.20);
  auto pt_bin = h_3D->GetYaxis()->FindBin(64213);
  auto ppt_bin = h_3D->GetZaxis()->FindBin(0.78);
  auto ppt_weight_1d = h_1D->GetBinContent(ppt_bin);
  auto ppt_weight_3d = h_3D->GetBinContent(eta_bin, pt_bin, ppt_bin);
  std::cout << "Weight 1D: " << ppt_weight_1d << std::endl;
  std::cout << "Weight 3D: " << ppt_weight_3d << std::endl;
  return 0;
}
