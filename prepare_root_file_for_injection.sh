#!/bin/bash

base_path=$HOME/eos/ttgamma_workspaces/ppt-3D-systs/
prompt_file=CR1_ttgamma_CR1_dilepton_ppt_2018-03-25_promptCR_3D_3slices_dummysyst/ttgamma_CR1_dilepton_ppt_2018-03-25_promptCR_3D_3slices_dummysyst/Histograms/ttgamma_CR1_dilepton_ppt_2018-03-25_promptCR_3D_3slices_dummysyst_ph_HFT_MVA_dilepton_ppt_histos.root
hfake_file=CR1_ttgamma_CR1_singlelepton_ppt_2018-03-25_hfakeCR_3D_3slices_dummysyst/ttgamma_CR1_singlelepton_ppt_2018-03-25_hfakeCR_3D_3slices_dummysyst/Histograms/ttgamma_CR1_singlelepton_ppt_2018-03-25_hfakeCR_3D_3slices_dummysyst_ph_HFT_MVA_singlelepton_ppt_histos.root

prompt_target=output_prompt.root
fake_target=output_fake.root

./derive-syst.exe "${base_path}${prompt_file}"
mv output.root ${prompt_target}
./derive-syst.exe "${base_path}${hfake_file}"
mv output.root ${fake_target}
hadd -f output_combined.root ${prompt_target} ${fake_target}
rm ${prompt_target} ${fake_target}
