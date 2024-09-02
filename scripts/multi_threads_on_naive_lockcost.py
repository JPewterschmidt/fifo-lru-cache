import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

df = pd.DataFrame([[1, 3.9], [2, 6.41], [3, 8.5], [4,8.42], 
                   [5,7.82], [6,11.68], [7,11.26], [8,11.72],
                   [9,10.86],[10,11.11], [11,11.52],[12,13.22],
                   [13,13.04],[14,12.69],[20,12.04],[30,12.42],
                   [40,14],[50,11.43]
                   ])

df.columns = ['thrnum', 'lockcost']

plt.figure(figsize=(10, 6))
sns.set_theme()
sns.lmplot(data=df, x='thrnum', y='lockcost', lowess=True, line_kws={'color': 'C1'})
ax = plt.gca()
ax.axvline(x=12,color='black',alpha=0.3)

plt.savefig('multi_threads_on_naive_lockcost.png', dpi=300, bbox_inches='tight')
