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

if len(df.columns) > 12:
    output = output + '2'

df2 = df
df1 = df.iloc[:12]

print('df1:')
print(df1)

print('df2:')
print(df2)

plt.figure(figsize=(10, 6))
sns.set_theme()
sns.scatterplot(data=df1, x=df1.index, y='cost', s=50)
plt.xlabel("Number of Threads")
plt.ylabel("Time Elapsed (ms)")
plt.xticks(range(1, len(df1) + 1))
plt.savefig(output + '.png', dpi=300, bbox_inches='tight')

if len(df2) <=12:
    quit()

plt.figure(figsize=(10, 6))
sns.set_theme()
sns.scatterplot(data=df2, x=df2.index, y='cost', s=50)
plt.xlabel("Number of Threads")
plt.ylabel("Time Elapsed (ms)")
plt.xticks(range(1, len(df2) + 1))

ax = plt.gca()
ax.axvline(x=12,color='black',alpha=0.3)

plt.savefig(output + '2.png', dpi=300, bbox_inches='tight')
