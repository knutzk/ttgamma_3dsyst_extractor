#ifndef _OUTPUT_HISTS_HH_
#define _OUTPUT_HISTS_HH_

#include <memory>
#include <string>
#include <vector>

#include "TH1F.h"
#include "TH3F.h"

class TFile;

class SystHist1D : public TH1F {
public:
  //! Initialise the 1D systematic histogram, with the path to
  //! the input file as input parameter.
  SystHist1D(const std::string& file_path);

  //! Default destructor.
  virtual ~SystHist1D() = default;

  //! Set the internal pointer for the container that contains
  //! strings of all MC histograms.
  void setMCHists(std::vector<std::string>* hists);

  //! Create the data/MC ratio from the input file and fill the
  //! histogram based on that ratio.
  void fillFromRatios();

private:
  //! Path to the input file.
  std::string file_path_;

  //! Pointer to the container with MC histogram names.
  std::vector<std::string>* mc_hists_;
};


//! Initialise the main output object (of class TH3F).
std::unique_ptr<TH3F> initOutput3DHist(const std::string& base_name);

void fill3D(TH3F* h_3D,
            const std::string& base_name,
            const std::vector<std::string>& eta_slice_strings,
            const std::vector<std::string>& pt_slice_strings,
            std::vector<std::string>* mc_hists,
            TFile* output_file);

#endif  // _OUTPUT_HISTS_HH_
