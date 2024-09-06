import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import sys

if len(sys.argv) <= 3:
    print('multi_thread_on_naive.py: provide csv and output path!, need 3 args, you have provided', len(sys.argv) - 1)
    quit()

csv1name = sys.argv[1]
csv2name = sys.argv[2]
csv3name = sys.argv[3]
output = sys.argv[4]
y_label = sys.argv[5]
need_draw_throughput = int(sys.argv[6])

df1 = pd.read_csv(csv1name)
df2 = pd.read_csv(csv2name)
df3 = pd.read_csv(csv3name)

df1 = pd.DataFrame(df1.mean())
df2 = pd.DataFrame(df2.mean())
df3 = pd.DataFrame(df3.mean())

df1.columns = ['cost']
df2.columns = ['cost']
df3.columns = ['cost']

df1_12only = df1.iloc[:12]
df2_12only = df2.iloc[:12]
df3_12only = df3.iloc[:12]

def draw_latency(df1, df2, df3, postfix='_latency.png'):
    plt.figure(figsize=(10, 6))
    sns.set_theme()
    sns.scatterplot(data=df1, x=df1.index, y='cost', s=50)
    sns.scatterplot(data=df2, x=df2.index, y='cost', s=50)
    sns.scatterplot(data=df3, x=df3.index, y='cost', s=50)

    sns.lineplot(data=df1, x=df1.index, y='cost')
    sns.lineplot(data=df2, x=df2.index, y='cost')
    sns.lineplot(data=df3, x=df3.index, y='cost')

    plt.xlabel("Number of Threads")
    plt.ylabel(y_label)
    plt.xticks(range(1, len(df1) + 1))
    plt.savefig(output + postfix, dpi=300, bbox_inches='tight')


def draw_throughput(df1, df2, df3, postfix='_throughput.png'):
    if (need_draw_throughput == 0):
        return

    df1 = (1000000 / df1) * 1000
    df2 = (1000000 / df2) * 1000
    df3 = (1000000 / df3) * 1000
    df1.columns = ['throughput']
    df2.columns = ['throughput']
    df3.columns = ['throughput']

    plt.figure(figsize=(10, 6))
    sns.set_theme()
    sns.scatterplot(data=df1, x=df1.index, y='throughput', s=50)
    sns.scatterplot(data=df2, x=df2.index, y='throughput', s=50)
    sns.scatterplot(data=df3, x=df3.index, y='throughput', s=50)

    sns.lineplot(data=df1, x=df1.index, y='throughput')
    sns.lineplot(data=df2, x=df2.index, y='throughput')
    sns.lineplot(data=df3, x=df3.index, y='throughput')

    plt.xlabel("Number of Threads")
    plt.ylabel("Thoughput (MQPS)")
    plt.xticks(range(1, len(df1) + 1))
    plt.savefig(output + postfix, dpi=300, bbox_inches='tight')


draw_latency(df1_12only, df2_12only, df3_12only)
draw_throughput(df1_12only, df2_12only, df3_12only)

if len(df1) <= 12:
    quit()

draw_latency(df1, df2, df3, '_timecost2.png')
draw_throughput(df1, df2, df3, '_throughput2.png')
