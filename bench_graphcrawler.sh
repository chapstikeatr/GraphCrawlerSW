#!/bin/bash
#SBATCH --job-name=graphCrawler_bench
#SBATCH --error=graphCrawler_%j.err
#SBATCH --out=graphCrawler_%j.out
#SBATCH --time=05:00:00
#SBATCH --partition=Centaurus
#SBATCH --mem=10G

srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 2 8
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 3 8
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 4 8
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 2 20
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 3 20
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 4 20
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 2 50
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 3 50
srun $HOME/GraphCrawlerSW/Parallel/paraLC "Tom Hanks" 4 50
srun $HOME/GraphCrawlerSW/sequential/seqLC "Tom Hanks" 2
srun $HOME/GraphCrawlerSW/sequential/seqLC "Tom Hanks" 3
srun $HOME/GraphCrawlerSW/sequential/seqLC "Tom Hanks" 4
