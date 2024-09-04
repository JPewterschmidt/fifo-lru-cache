import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import sys

if len(sys.argv) <= 3:
    print('multi_thread_on_naive.py: provide csv and output path!, need 3 args, you have provided', len(sys.argv) - 1)
    quit()

csv1name = sys.argv[1]
csv2name = sys.argv[2]
output = sys.argv[3]

df1 = pd.read_csv(csv1name)
df2 = pd.read_csv(csv2name)
df1 = pd.DataFrame(df1.mean())
df2 = pd.DataFrame(df2.mean())
df1.columns = ['cost']
df2.columns = ['cost']

df1_12only = df1.iloc[:12]
df2_12only = df2.iloc[:12]

def draw(df1, df2, postfix='.png'):
    plt.figure(figsize=(10, 6))
    sns.set_theme()
    sns.scatterplot(data=df1, x=df1.index, y='cost', s=50)
    sns.scatterplot(data=df2, x=df1.index, y='cost', s=50)

    sns.lineplot(data=df1, x=df1.index, y='cost')
    sns.lineplot(data=df2, x=df1.index, y='cost')

    plt.xlabel("Number of Threads")
    plt.ylabel("Time Elapsed (ms)")
    plt.xticks(range(1, len(df1) + 1))
    plt.savefig(output + postfix, dpi=300, bbox_inches='tight')


draw(df1_12only, df2_12only)

if len(df1) <= 12:
    quit()

draw(df1, df2, '2.png')
