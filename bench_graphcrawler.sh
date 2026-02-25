#!/bin/bash
#SBATCH --job-name=graphCrawler_bench
#SBATCH --error=graphCrawler_%j.err
#SBATCH --out=graphCrawler_%j.out
#SBATCH --time=05:00:00
#SBATCH --partition=Centaurus
#SBATCH --mem=10G

srun $HOME/GraphCrawlerSW/static_work_graphcrawler/sequential/ "Tom Hanks" 2
srun $HOME/GraphCrawlerSW/static_work_graphcrawler/sequential/ "Tom Hanks" 2
srun $HOME/GraphCrawlerSW/static_work_graphcrawler/sequential/ "Tom Hanks" 2
srun $HOME/GraphCrawlerSW/static_work_graphcrawler/Parallel/paraLC"Tom Hanks" 2
srun $HOME/GraphCrawlerSW/static_work_graphcrawler/Parallel/paraLC"Tom Hanks" 3
srun $HOME/GraphCrawlerSW/static_work_graphcrawler/Parallel/paraLC"Tom Hanks" 4
srun $HOME/GraphCrawlerSW/graphCrawler "Tom Hanks" 3
srun $HOME/GraphCrawlerSW/graphCrawler "Tom Hanks" 4
