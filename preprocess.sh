#!/bin/bash
#SBATCH --mem 100G -J preprocess_smoother --time=240:00:00 -o slurm_preprocess_heatmap-%j.out --mail-user=markus.rainer.schmidt@gmail.com --mail-type END


source activate $(pwd)/conda_env/smoother

#./bin/conf_version.sh
#cat VERSION

BAMS="/work/project/ladsie_012/ABS.2.2/20210608_Inputs"
BAM_SUF="R1.sorted.bam"

INDEX_PREFIX="../smoother_out/radicl"

rm -r ${INDEX_PREFIX}.smoother_index

echo "working on index ${INDEX_PREFIX}"


python3 python/main.py indexer init "${INDEX_PREFIX}" "../smoother_in/Lister427.sizes" -d 1000

python3 python/main.py indexer anno "${INDEX_PREFIX}" "../smoother_in/HGAP3_Tb427v10_merged_2021_06_21.gff3"

python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS503_P10_Total_2.${BED_SUF}" "P10_Total_Rep2" -g a

python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS617_P10_Total_1.${BED_SUF}" "P10_Total_Rep1" -g a
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS504_P10_Total_3.${BED_SUF}" "P10_Total_Rep3" -g a
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS505_N50_Total_1.${BED_SUF}" "N50_Total_Rep1" -g a
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS506_N50_Total_2.${BED_SUF}" "N50_Total_Rep2" -g a
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS507_N50_Total_3.${BED_SUF}" "N50_Total_Rep3" -g a

python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS508_P10_NPM_1.${BED_SUF}" "P10_NPM_Rep1" -g b

python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS509_P10_NPM_2.${BED_SUF}" "P10_NPM_Rep2" -g b
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS510_P10_NPM_3.${BED_SUF}" "P10_NPM_Rep3" -g b
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS511_N50_NPM_1.${BED_SUF}" "N50_NPM_Rep1" -g b
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS512_N50_NPM_2.${BED_SUF}" "N50_NPM_Rep2" -g b
python3  python/main.py indexer repl "${INDEX_PREFIX}" "${BEDS}/NS513_N50_NPM_3.${BED_SUF}" "N50_NPM_Rep3" -g b

