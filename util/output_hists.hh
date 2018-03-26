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
  //! Inherit all constructors from the base class.
  using TH1F::TH1F;

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


class SystHist3D : public TH3F {
 public:
  //! Inherit all constructors from the base class.
  using TH3F::TH3F;

  //! Initialise the 3D systematic histogram, with the path to
  //! the main input file as input parameter.
  SystHist3D(const std::string& file_path);

  //! Default destructor.
  virtual ~SystHist3D() = default;

  //! Set the internal pointer for the container that contains
  //! strings of all MC histograms.
  void setMCHists(std::vector<std::string>* hists);

  //! Set the internal pointer for the container that contains
  //! strings of all eta slices.
  void setEtaSlices(std::vector<std::string>* strings);

  //! Set the internal pointer for the container that contains
  //! strings of all pt slices.
  void setPtSlices(std::vector<std::string>* strings);

  //! Create the data/MC ratios from the input file and all its
  //! derivatives for the different eta/pt slices and fill the 3D
  //! histogram based on these ratios.
  void fillFromRatios();

 private:
  //! Path to the main input file. This file name serves as a
  //! base to derive the names of all other files.
  std::string file_path_;

  //! Pointer to the container with MC histogram names.
  std::vector<std::string>* mc_hists_;

  //! Pointer to the container with eta slice strings.
  std::vector<std::string>* eta_slices_;

  //! Pointer to the container with pt slice strings.
  std::vector<std::string>* pt_slices_;
};

#endif  // _OUTPUT_HISTS_HH_
