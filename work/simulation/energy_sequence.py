import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib as mplib
import matplotlib.gridspec as gridspec
import sys
import matplotlib.font_manager as fm

mplib.rcParams['font.size'] = 6 

if len(sys.argv) ==3:
    tail = sys.argv[2]
    excel_path = f"out_{tail}/"
else:
    excel_path = f"out/"
excel_file_name = "parallel.csv"

df = pd.read_csv(excel_path + excel_file_name)
df_sorted = df.sort_values(by='i', ascending=True)
df_sorted = df_sorted.drop_duplicates(subset='i', keep='first')
if len(sys.argv) > 1:
    upper_limit = int(sys.argv[1])
    df_sorted = df_sorted[df_sorted['i'] <= upper_limit]

df_sorted['concave_norm'] = df_sorted['concave'] / df_sorted['concave'].max()
df_sorted['convex_norm'] = df_sorted['convex'] / df_sorted['convex'].max()

window_size = 10
df_sorted['concave_MA_norm'] = df_sorted['concave_norm'].rolling(window=window_size).mean()
df_sorted['convex_MA_norm'] = df_sorted['convex_norm'].rolling(window=window_size).mean()

plt.figure(figsize=(400/25.4,300/25.4))
plt.gca().set_aspect(500)

try:
    component = int(input("which component (1: concave/2: comvex)?"))
    if component == 1:
        plt.plot(df_sorted['i'], df_sorted['concave_norm'], label='Concave', color="dimgrey",
                alpha=0.3, linewidth=1) 
        plt.plot(df_sorted['i'], df_sorted['concave_MA_norm'], label='Concave Moving Average', color="dimgrey",#赤紫
                linewidth=2) 
    elif component == 2:
        plt.plot(df_sorted['i'], df_sorted['convex_norm'], label='Convex', color="dimgrey",
                alpha=0.3, linewidth=1) 
        plt.plot(df_sorted['i'], df_sorted['convex_MA_norm'], label='Convex Moving Average', color="dimgrey",#緑紫
                linewidth=2) 
    else: raise ValueError
except ValueError as e:
    print("Input must be 1 or 2.")

plt.ylim(0.5,1.05)

plt.tick_params(width = 1, length = 1)
plt.xticks(np.arange(df_sorted['i'].min(), df_sorted['i'].max()+10, step=10)) 
plt.yticks(np.arange(0.5, 1.05, step=0.2))

plt.grid(True, which='both', linestyle='--', linewidth=0.7, color='gray')

# plt.xlabel('Step', fontsize=6)
#plt.ylabel('Energy', fontsize=6)
#plt.legend()
plt.grid(True)

plt.tight_layout()

save = True
save_path = ".//"
save_file_name = "energy_sequence"
if save:
    plt.savefig(save_path + save_file_name + ".pdf")
    #plt.savefig(save_path + save_file_name + ".svg")

plt.show()

plt.close()
