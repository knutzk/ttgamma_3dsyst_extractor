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

//! Initialise the main output object (of class TH3F).
std::unique_ptr<TH3F> initOutput3DHist(const std::string& base_name);

void fill3D(TH3F* h_3D,
            const std::string& base_name,
            const std::vector<std::string>& eta_slice_strings,
            const std::vector<std::string>& pt_slice_strings,
            std::vector<std::string>* mc_hists,
            TFile* output_file);


class SystHist1D : public TH1F {
public:
  SystHist1D(const std::string& file_path);
  virtual ~SystHist1D() = default;

  void setMCHists(std::vector<std::string>* hists);
  void fillFromRatios();

private:
  std::string file_path_;
  std::vector<std::string>* mc_hists_;
};

#endif  // _OUTPUT_HISTS_HH_
