import numpy as np
from numpy.lib.function_base import angle
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib as mplib
import matplotlib.gridspec as gridspec
import sys

from matplotlib.colors import LinearSegmentedColormap

mplib.rcParams['font.size'] = 18  # 全体のフォントサイズを設定

'''
excel_path = "out/"
excel_file_name = "parallel.csv"
if len(sys.argv) > 1:
    excel_file_name = "parallel_till_" + sys.argv[1] + ".csv"
df = pd.read_csv(excel_path + excel_file_name)
'''
def load_data(n1, n2, excel_path="out/", output_path="output_diff.csv"):
    # CSVファイルを読み込む
    df_n1 = pd.read_csv(excel_path + f"angle_distr_parallel_till_{n1}.csv")
    df_n2 = pd.read_csv(excel_path + f"angle_distr_parallel_till_{n2}.csv")
    
    # n2からn1の差分データを抽出する
    if len(df_n2) > len(df_n1):
        df_diff = df_n2[len(df_n1):]  # df_n1よりも後に追加された行を取得
    else:
        df_diff = pd.DataFrame()  # もし df_n1 の方が行数が多ければ、空のデータフレームを返す
    
    df_diff.to_csv(output_path, index=False)
    return df_diff

# 角度の度数分布表を作成する関数
def create_angular_frequency_distribution_table(df_diff, df_base, div_num, MAX_DEGREE=90):
    # １つのエリアの幅を計算する。(ex. MAX_DEGREE=180°かつdiv_num=9の場合, 1つのエリアの幅は20°)
    each_angle_size = MAX_DEGREE / div_num
    # 各エリアの角度を入れる。(+1により最後の角度も入る)
    angle_list = [i * each_angle_size for i in range(div_num + 1)]
    # 度数分布表を辞書型で初期化する
    angular_distribution = {angle: 0. for angle in angle_list[:-1]}
    term_color = {angle: 0. for angle in angle_list[:-1]}
    count = {angle: 0 for angle in angle_list[:-1]}
    # dataframeを一行ずつ読み込む
    for index, row in df_diff.iterrows():
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
    
    #max_colorの計算（ここはinputに依存させない）
    angular_distribution_base = {angle: 0. for angle in angle_list[:-1]}
    #term_color_base = {angle: 0. for angle in angle_list[:-1]}

    for index, row in df_base.iterrows():
        # ある行の角度が度数分布表のどこに入るかを探す
        for i in range(len(angle_list)-1):
            # もしある行の角度が, angle_list[i]以上angle_list[i+1]未満ならその度数分布表を+1してループを抜ける
            if ((row["angle"] /np.pi*180 >= angle_list[i]) and (row["angle"]/ np.pi*180  < angle_list[i+1])):
                angular_distribution_base[angle_list[i]] += row["norm"]
                #term_color_base[angle_list[i]] += row["term"]
                break
    
    max_color = 0.
    for i in range(len(angle_list) - 1):
        max_color = max(term_color[angle_list[i]], max_color)
    assert(max_color != 0)
    for i in range(len(angle_list) - 1):
        term_color[angle_list[i]] /= max_color
    return [angular_distribution, angular_distribution_base, angle_list, term_color]

def plot_angular_distribution(ax, angle_dist, angle_list, color, colormap, r_max, r_max_base, r_tick_width, MAX_DEGREE=90):
    import math
    mplib.rcParams['font.size'] = 6
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
    ax.set_ylim([0, r_max_base])
    #　r_tick_width刻みにtickを入れる
    ax.set_yticks([r_tick_width * i for i in range(int(math.ceil(r_max_base/r_tick_width)))])

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
        # ax.fill(theta, r_fill, color=fill_color) #"#aaaaaa")
        ax.fill(theta, r_fill, color="cyan") #"#aaaaaa")


    return ax

def main(n1, n2):
    mplib.rcParams['font.size'] = 6
    # 差分データを読み込む
    df_diff = load_data(n1, n2, excel_path="out/", output_path="output_diff.csv")

    df_base = pd.read_csv(f"out/angle_distr_parallel.csv") #作図用

    # 差分データを元に度数分布を計算
    angle_dist, angle_dist_base, angle_list, color = create_angular_frequency_distribution_table(df_diff, df_base, 9)

    # ここからグラフを作成
    # # 1行2列のグラフの土台を作成
    fig = plt.figure(figsize=(40/25.4,30/25.4))
    spec = gridspec.GridSpec(ncols=2, nrows=1,
                             width_ratios=[50, 1])
    
    # 図のサイズ
    #fig.set_size_inches(38.75/25.4*4, 26.25/25.4*4)

    ax = fig.add_subplot(projection="polar")
    # cb = fig.add_subplot(spec[1])
    # グラフのサイズ指定
    #fig.set_size_inches(12, 5)

    # df_Aからangular_distributionとangle_listを計算する
    # angle_dist, angle_list, color= create_angular_frequency_distribution_table(df_diff, 9)

    max_r = 0;
    max_r = max(angle_dist[angle_list[i]] for i in range(len(angle_list) - 1))
    #print(max_r)

    #ylim
    max_r_base = 0;
    max_r_base = max(angle_dist_base[angle_list[i]] for i in range(len(angle_list) - 1))
    import math
    r_sum = 0.;
    for i in range(len(angle_dist_base) - 1):
        r_sum += angle_dist_base[angle_list[i]]
    print(r_sum)

    max_r_base = math.ceil(max_r_base/r_sum*10) * 10

    # 1行1列目のグラフにプロット
    
    #colormap = cm.Purples

    # カスタムの紫色のカラーマップを定義（中明度の紫色と白の間のグラデーション）
    custom_cmap = LinearSegmentedColormap.from_list("custom_purple", ["#ffffff", "cyan"])
    # カラーマップとして反映
    colormap = custom_cmap

    #print(color)
    ax = plot_angular_distribution(ax, angle_dist, angle_list, color, colormap, max_r, max_r_base, 5)
    ax.set_facecolor(color='white')
    ax.set_theta_direction(-1)
    ax.set_theta_zero_location('N')
    #ax.set_ylabel("Shape change angle [%]", labelpad=0, fontsize=6, color='black')
    #ax.set_rgrids([0,5,10,15], angle=90)
    # 角度ラベルの位置を外側に移動
    ax.tick_params(pad=-2)  # デフォルトよりも15ピクセル外側に移動

    for spine in ax.spines.values():
        spine.set_edgecolor('black')

    ax.tick_params(colors='black')

    # norm = mplib.colors.Normalize(vmin=0, vmax=1)
    # cbar = mplib.colorbar.ColorbarBase(
    #     ax=cb,
    #     cmap=colormap,
    #     norm=norm,
    #     orientation="vertical"
    #     #ticks=[0,1],
    # )
    # cbar.set_label("Average contact force",
    #     labelpad=18, color='black')
    # cbar.set_ticks([])
    # cbar.ax.text(1.5, 1.0, 'Max', ha='center', va='bottom', color='black', transform=cbar.ax.transAxes,fontsize=18)
    # cbar.ax.text(1.5, 0.0, 'Min', ha='center', va='top', color='black', transform=cbar.ax.transAxes,fontsize=18)
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

    # tight_layoutで余白を調整
    plt.tight_layout()


    # saveするかどうかのboolean
    save = True
    # 保存先のpathとファイル名を指定
    save_path = ".//"
    save_file_name = "angle_distribution_plot"
    if len(sys.argv) == 3:
        save_file_name = "angle_distribution_from_" + sys.argv[1] + "_to_" + sys.argv[2]
    if save==True:
        plt.savefig(save_path + save_file_name + ".pdf")
        #plt.savefig(save_path + save_file_name + ".svg")

    # グラフを見る
    #plt.show()
    # グラフの情報を捨てる
    plt.close()

# 実行部分: コマンドライン引数からn1, n2を取得して差分データをプロット
if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python your_script.py <n1> <n2>")
        sys.exit(1)

    try:
        n1 = int(sys.argv[1])
        n2 = int(sys.argv[2])
    except ValueError:
        print("Both n1 and n2 must be integers.")
        sys.exit(1)

    main(n1, n2)