import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import sys



if (sys.argv.__len__() < 4):
    print(f"usage. {sys.argv[0]:} matches literals")
    sys.exit(0)


#sns.set_theme(style="darkgrid")
sns.set_theme(style="ticks")

# matches and offsets
matches = pd.read_csv(sys.argv[1],header="infer")
#sns.relplot(data=matches, x="Offset", y="Length")
g = sns.jointplot(x="Offset", y="Length", data=matches,
                  kind="reg", truncate=False,
                  xlim=(0, 18000), ylim=(2, 172),
                  color="m", height=7)

#g = sns.JointGrid(data=matches, x="Offset", y="Length")
#g.plot_joint(sns.histplot)
#g.plot_marginals(sns.boxplot)


# macth length histogram

arr = np.array(range(2,200,8))
sns.displot(matches,x="Length",bins=arr,log_scale=(False,2))
plt.xticks(arr)

#
literals = pd.read_csv(sys.argv[2],header="infer")
arr = np.array(range(0,256))
sns.displot(literals,x="Literals",bins=arr,log_scale=(False,2))
#plt.xticks(arr)

literal_runs = pd.read_csv(sys.argv[3],header="infer")
arr = np.array(range(0,100))
sns.displot(literal_runs,x="Literal_runs",bins=arr,log_scale=(False,2))
#plt.xticks(arr)





#sns.displot(matches,x="Length",bins=16)

# show
plt.show()






#


