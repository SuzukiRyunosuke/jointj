import numpy as np
from numpy.lib.function_base import angle
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib as mplib
import matplotlib.gridspec as gridspec
import sys
import matplotlib.font_manager as fm

mplib.rcParams['font.size'] = 6  # 全体のフォントサイズを設定

# データファイルのパス
if len(sys.argv) ==3:
    tail = sys.argv[2]
    excel_path = f"out_{tail}/"
else:
    excel_path = f"out/"
excel_file_name = "parallel.csv"

# データ読み込み
df = pd.read_csv(excel_path + excel_file_name)
# 'i' 列を昇順に並べ替え
df_sorted = df.sort_values(by='i', ascending=True)
# 'i' 列の重複を削除（最初の1つだけを残す）
df_sorted = df_sorted.drop_duplicates(subset='i', keep='first')
# 'i' 列がsys.argv[1]を超える行を削除
if len(sys.argv) > 1:
    upper_limit = int(sys.argv[1])
    df_sorted = df_sorted[df_sorted['i'] <= upper_limit]

# concave と convex の両方での最大値を取得
# max_value = max(df_sorted['concave'].max(), df_sorted['convex'].max())

# 正規化 (両者の最大値で割る)
df_sorted['concave_norm'] = df_sorted['concave'] / df_sorted['concave'].max()
df_sorted['convex_norm'] = df_sorted['convex'] / df_sorted['convex'].max()

# 移動平均の計算（例: 10点移動平均）
window_size = 10
df_sorted['concave_MA_norm'] = df_sorted['concave_norm'].rolling(window=window_size).mean()
df_sorted['convex_MA_norm'] = df_sorted['convex_norm'].rolling(window=window_size).mean()

# プロットの作成
plt.figure(figsize=(40/25.4,30/25.4))
plt.gca().set_aspect(229)

# 薄い線で元のデータ
plt.plot(df_sorted['i'], df_sorted['concave_norm'], label='Concave', color="dimgrey",#赤紫#93878F
         alpha=0.3, linewidth=1) 
# plt.plot(df_sorted['i'], df_sorted['convex_norm'], label='Convex', color="dimgrey",#緑紫#7E8C84
#           alpha=0.3, linewidth=1) 

# 濃い線で移動平均
plt.plot(df_sorted['i'], df_sorted['concave_MA_norm'], label='Concave Moving Average', color="dimgrey",#赤紫
          linewidth=2) 
# plt.plot(df_sorted['i'], df_sorted['convex_MA_norm'], label='Convex Moving Average', color="dimgrey",#緑紫
#           linewidth=2) 

plt.ylim(0,1.05)


plt.tick_params(width = 1, length = 1)
# x軸とy軸の目盛りの間隔を指定
plt.xticks(np.arange(df_sorted['i'].min(), df_sorted['i'].max()+10, step=80))  # 50刻みの間隔を設定
plt.yticks(np.arange(0, 1.05, step=0.2))  # 0.1刻みの間隔を設定（正規化されているため0〜1の範囲）

# グリッドを太く、目立つ色で表示
plt.grid(True, which='both', linestyle='--', linewidth=0.7, color='gray')

#その他
#plt.title('Data and Moving Averages')
plt.xlabel('Step', fontsize=6)
#plt.ylabel('Energy', fontsize=6)
#plt.legend()
plt.grid(True)

# tight_layoutで余白を調整
plt.tight_layout()

# saveするかどうかのboolean
save = True
# 保存先のpathとファイル名を指定
save_path = ".//"
save_file_name = "energy_sequence"
if save:
    plt.savefig(save_path + save_file_name + ".pdf")
    #plt.savefig(save_path + save_file_name + ".svg")

plt.show()

plt.close()
