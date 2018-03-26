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
                               const std::string& pt_slice_string);

//! Create the correct base string for histogram for a given file name. This
//! essentially cuts away some parts of the given string.
std::string createHistString(const std::string& str);

//! Initialise the main output object (of class TH3F).
std::unique_ptr<TH3F> initOutput3DHist(const std::string& base_name);

//! Get a histogram from a file. Throw an exception otherwise.
TH1F* getHistogram(TFile* file, const std::string& hist_string);

//! Prepare a data/MC ratio, with MC scaled to data.
std::unique_ptr<TH1F> prepareDataMCRatio(TFile* file,
                                         const std::string& hist_string,
                                         std::vector<std::string>* mc_hists);

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

void fill3D(TH3F* h_3D,
            const std::string& base_name,
            const std::vector<std::string>& eta_slice_strings,
            const std::vector<std::string>& pt_slice_strings,
            std::vector<std::string>* mc_hists,
            TFile* output_file);


#endif  // _OUTPUT_HISTS_HH_
