import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import sys

if len(sys.argv) <= 2:
    print('multi_thread_on_naive.py: provide csv and output path!, need 2 args, you have provided', len(sys.argv) - 1)
    quit()

csvname = sys.argv[1]
output = sys.argv[2]
df = pd.read_csv(csvname)
df = pd.DataFrame(df.mean())
df.columns = ['cost']

plt.figure(figsize=(10, 6))
sns.scatterplot(data=df, x=df.index, y='cost', color='red', s=50)
plt.xlabel("Thread Number")
plt.ylabel("Time Elapsed (ms)")
plt.xticks(range(1, len(df) + 1))
plt.savefig(output, dpi=300, bbox_inches='tight')
