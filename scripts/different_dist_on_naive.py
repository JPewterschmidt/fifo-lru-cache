import matplotlib.pyplot as plt
import pandas as pd
import sys

if len(sys.argv) <= 2:
    print('different_dist_on_naive.py: provide csv and output path!, need 2 args, you have provided', len(sys.argv) - 1)
    quit()

csvname = sys.argv[1]
output = sys.argv[2]
df = pd.read_csv(csvname)

fig, ax = plt.subplots(figsize=(4,3))
ax.bar(df['dist'], df['hits'], color='red', label='Hits', alpha=0.9)
ax.bar(df['dist'], df['misses'], bottom=df['hits'], color='blue', label='Misses', alpha=0.9)
ax.set_ylabel('%')
ax.set_title('Hits/Misses')
ax.set_xticks(range(len(df['dist'])))
ax.set_xticklabels(df['dist'], rotation=45)
ax.legend()

for i, row in df.iterrows(): 
    x = i
    y = row['hits'] + 5
    hit_ratio_text = f"{row['hit-ratio']:.2f}"
    ax.text(x, y, hit_ratio_text, ha='center', va='bottom', fontsize=10, color='white')

plt.tight_layout()
plt.savefig(output, dpi=300, bbox_inches='tight')
