import numpy as np
from numpy.lib.function_base import angle
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib as mplib
import matplotlib.gridspec as gridspec
import sys

excel_path = "../out/"
excel_file_name = "gdn_parallel.csv"
if len(sys.argv) > 1:
    excel_file_name = "gdn_parallel_till_" + sys.argv[1] + ".csv"
df = pd.read_csv(excel_path + excel_file_name)
# 角度の度数分布表を作成する関数
def create_angular_frequency_distribution_table(df, div_num, MAX_DEGREE=90):
    # １つのエリアの幅を計算する。(ex. MAX_DEGREE=180°かつdiv_num=9の場合, 1つのエリアの幅は20°)
    each_angle_size = MAX_DEGREE / div_num
    # 各エリアの角度を入れる。(+1により最後の角度も入る)
    angle_list = [i * each_angle_size for i in range(div_num + 1)]
    # 度数分布表を辞書型で初期化する
    angular_distribution = {angle: 0. for angle in angle_list[:-1]}
    term_color = {angle: 0. for angle in angle_list[:-1]}
    count = {angle: 0 for angle in angle_list[:-1]}
    # dataframeを一行ずつ読み込む
    for index, row in df.iterrows():
        # ある行の角度が度数分布表のどこに入るかを探す
        for i in range(len(angle_list)-1):
            # もしある行の角度が, angle_list[i]以上angle_list[i+1]未満ならその度数分布表を+1してループを抜ける
            if ((row["angle"] /np.pi*180 >= angle_list[i]) and (row["angle"]/ np.pi*180  < angle_list[i+1])):
                count[angle_list[i]] += 1
                angular_distribution[angle_list[i]] += row["norm"]
                term_color[angle_list[i]] += row["term"]
                break

    for i in range(len(angle_list) - 1):
        if (angular_distribution[angle_list[i]] > 0):
            term_color[angle_list[i]] /= count[angle_list[i]]
    max_color = 0.
    for i in range(len(angle_list) - 1):
        max_color = max(term_color[angle_list[i]], max_color)
    assert(max_color != 0)
    #max_color *= 1.1
    for i in range(len(angle_list) - 1):
        term_color[angle_list[i]] /= max_color
    return [angular_distribution, angle_list, term_color]

def plot_angular_distribution(ax, angle_dist, angle_list, color, colormap, r_max, r_tick_width, MAX_DEGREE=90):
    import math
    r_sum = 0.;
    for i in range(len(angle_dist) - 1):
        r_sum += angle_dist[angle_list[i]]

    r_max = math.ceil(r_max/r_sum*10) * 10
    #ax.set_rscale("symlog")
    # 偏角Θの範囲を決める
    ax.set_xlim([0, np.pi*MAX_DEGREE/180])
    # 30°刻みにtickを入れる
    ax.set_xticks([i * np.pi / 6 for i in range(int(math.floor(MAX_DEGREE/30)+1))])
    # 動径rの範囲を引数r_maxから設定する
    ax.set_ylim([0, r_max])
    #　r_tick_width刻みにtickを入れる
    ax.set_yticks([r_tick_width * i for i in range(int(math.ceil(r_max/r_tick_width)))])

    for i in range(len(angle_list)-1):
        # 扇形の閉曲線を作る
        theta = np.array(
            [angle_list[i]*np.pi/180, angle_list[i]*np.pi/180] + 
            [theta*np.pi/180 for theta in np.linspace(angle_list[i], angle_list[i+1], num=int(angle_list[i+1]-angle_list[i]))] +
            [angle_list[i+1]*np.pi/180, angle_list[i+1]*np.pi/180]
        )
        r = np.array(
            [0, angle_dist[angle_list[i]]] + 
            [angle_dist[angle_list[i]] for theta in np.linspace(angle_list[i], angle_list[i+1], num=int(angle_list[i+1]-angle_list[i]))] + 
            [angle_dist[angle_list[i]], 0]
        )
        r_normalized = r / r_sum * 100;

        # 作った扇形をプロット
        ax.plot(theta, r_normalized, color="#333333", linewidth=1)
        # 扇の内側を塗りつぶす
        #fill_color = colormap(color[angle_list[i]])
        #back_color = colormap(0.)
        #ax.fill(theta, r_normalized, color=back_color) #"#aaaaaa")
        fill_color = colormap(color[angle_list[i]])
        r_fill = r_normalized #* color[angle_list[i]]
        ax.fill(theta, r_fill, color=fill_color) #"#aaaaaa")

    return ax


# ここからグラフを作成
# 1行2列のグラフの土台を作成
fig = plt.figure()
spec = gridspec.GridSpec(ncols=2, nrows=1,
                         width_ratios=[50, 1])
ax = fig.add_subplot(spec[0], projection="polar")
cb = fig.add_subplot(spec[1])
# グラフのサイズ指定
#fig.set_size_inches(12, 5)

# df_Aからangular_distributionとangle_listを計算する
angle_dist, angle_list, color= create_angular_frequency_distribution_table(df, 9)

max_r = 0;
for i in range(len(angle_list) - 1):
    max_r = max(angle_dist[angle_list[i]], max_r)
print(max_r)
# 1行1列目のグラフにプロット
colormap = cm.Oranges
print(color)
ax = plot_angular_distribution(ax, angle_dist, angle_list, color, colormap, max_r, 5)
ax.set_theta_direction(-1)
ax.set_theta_zero_location('N')
ax.set_ylabel("Rate of Shape Change Amount (%)")
#ax.set_rgrids([0,5,10,15], angle=90)

norm = mplib.colors.Normalize(vmin=0, vmax=1)
cbar = mplib.colorbar.ColorbarBase(
    ax=cb,
    cmap=colormap,
    norm=norm,
    orientation="vertical",
    label="Contact Force Avg.",
    ticks=[0,1],
)
cbar.set_ticklabels(ticklabels=["min", "max"])
# df_Bからangular_distributionとangle_listを計算する
#angle_dist_B, angle_list_B = create_angular_frequency_distribution_table(df_B, 9)
# 1行2列目のグラフにプロット
#axes[1] = plot_angular_distribution(axes[1], angle_dist_B, angle_list_B, 22, 10)

#or ax, name in zip(axes, ["Sample A", "Sample B"]):
    # タイトルをフォントサイズ14で"Sample B"とし、位置調整
#ax.set_title(name, y=0.85, fontsize=14)
    # xlabelをフォントサイズ14で"Frequency"に
#ax.set_xlabel("Angle", fontsize=14)
    # xlabelの位置調整
#ax.xaxis.set_label_coords(0.5, 0.15)
#ig.tight_layout()

# saveするかどうかのboolean
save = True
# 保存先のpathとファイル名を指定
save_path = ".//"
save_file_name = "gdn_angle_distribution_plot"
if len(sys.argv) > 1:
    save_file_name = "gdn_angle_distribution_till_" + sys.argv[1]
if save==True:
    plt.savefig(save_path + save_file_name + ".png")
    #plt.savefig(save_path + save_file_name + ".svg")

# グラフを見る
#plt.show()
# グラフの情報を捨てる
plt.close()