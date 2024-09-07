import matplotlib.pyplot as plt
import pandas as pd
import sys

if len(sys.argv) <= 2:
    print('different_dist_on_naive.py: provide csv and output path!, need 2 args, you have provided', len(sys.argv) - 1)
    quit()

csvname = sys.argv[1]
output = sys.argv[2]
df = pd.read_csv(csvname)

fig, ax = plt.subplots(figsize=(2.25, 2.725))
ax.bar(df['dist'], df['hits'], label='Hits')
ax.bar(df['dist'], df['misses'], bottom=df['hits'], label='Misses')
#ax.bar(df['dist'], df['hits'], color='red', label='Hits', alpha=0.7)
#ax.bar(df['dist'], df['misses'], bottom=df['hits'], color='blue', label='Misses', alpha=0.7)
ax.set_ylabel('Hit Ratios')
ax.set_yticks([])
ax.set_xticks(range(len(df['dist'])))
ax.set_xticklabels(df['dist'], rotation=45)
ax.legend()

for i, row in df.iterrows(): 
    x = i
    y = row['hits'] - 1e6 * 1.3
    hit_ratio_text = f"{row['hit-ratio']:.3f}"
    ax.text(x, y, hit_ratio_text, ha='center', va='bottom', fontsize=10, color='white')

plt.tight_layout()
plt.savefig(output, dpi=300, bbox_inches='tight')
