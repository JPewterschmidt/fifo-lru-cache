import matplotlib.pyplot as plt
import pandas as pd
import sys

if len(sys.argv) <= 1:
    print('provide differnent_dist_on_naive.csv path!')
    quit()

filename = sys.argv[1] + '/differnent_dist_on_naive.csv'
df = pd.read_csv(filename)

fig, ax = plt.subplots(figsize=(4,3))
ax.bar(df['dist'], df['hits'], color='red', label='Hits')
ax.bar(df['dist'], df['misses'], bottom=df['hits'], color='blue', label='Misses')
ax.set_ylabel('%')
ax.set_title('Hits/Misses')
ax.set_xticks(range(len(df['dist'])))
ax.set_xticklabels(df['dist'], rotation=45)
ax.legend()

plt.tight_layout()
plt.savefig('pics/differnent_dist_on_naive.png', dpi=300, bbox_inches='tight')
