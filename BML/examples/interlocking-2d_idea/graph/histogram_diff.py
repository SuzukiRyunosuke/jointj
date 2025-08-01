import numpy as np
from numpy.lib.function_base import angle
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib as mplib
import matplotlib.gridspec as gridspec
import sys
import matplotlib.ticker as ticker
from matplotlib.ticker import FormatStrFormatter

def load_data_with_base(n1, n2, excel_path="out/", base_file="angle_distr_parallel.csv"):
    """
    差分データと基準データを読み込む
    Args:
        n1, n2: データファイル番号
        excel_path: ファイルパス
        base_file: 基準データファイル名
    Returns:
        df_diff: 差分データ
        df_base: 基準データ
    """
    df_n1 = pd.read_csv(excel_path + f"angle_distr_parallel_till_{n1}.csv")
    df_n2 = pd.read_csv(excel_path + f"angle_distr_parallel_till_{n2}.csv")
    df_base = pd.read_csv(excel_path + base_file)

    # 差分データの抽出
    if len(df_n2) > len(df_n1):
        df_diff = df_n2[len(df_n1):]
    else:
        df_diff = pd.DataFrame()

    return df_diff, df_base

def determine_term_range_and_bins(df_base, div_num=100, max_x=2.00):
    """
    基準データから term_range と bin 数を決定する
    Args:
        df_base: 基準データ
        div_num: ビンの数
    Returns:
        term_range: (min_term, max_term) の範囲
        div_num: 決定されたビンの数
    """
    df_base["term_normalized"] = df_base["term"] / max_x
    min_term = df_base["term_normalized"].min()
    max_term = df_base["term_normalized"].max()
    return (min_term, max_term), div_num

def create_term_frequency_distribution(df_diff, term_range, div_num, max_x=2.00):
    """
    term の度数分布を作成する
    Args:
        df_diff: 差分データ
        term_range: (min_term, max_term) の範囲
        div_num: ビンの数
    Returns:
        term_distribution: ビンごとの頻度
        bin_edges: ビン境界リスト
    """
    min_term, max_term = term_range
    bin_edges = np.linspace(min_term, max_term, div_num + 1)
    df_diff["term_normalized"] = df_diff["term"] / max_x
    term_distribution = np.histogram(df_diff["term_normalized"], bins=bin_edges)[0]
    return term_distribution, bin_edges

def create_normalized_cumulative_distribution(df_diff, term_range, div_num, max_x=2.00):
    """
    term の累積度数分布を作成し、累積度数分布の最大値で正規化する
    Args:
        df_diff: 差分データ
        term_range: (min_term, max_term) の範囲
        div_num: ビンの数
    Returns:
        normalized_cumulative_distribution: 正規化された累積度数分布
        bin_edges: ビン境界リスト
    """
    min_term, max_term = term_range
    bin_edges = np.linspace(min_term, max_term, div_num + 1)
    df_diff["term_normalized"] = df_diff["term"] / max_x
    term_distribution, _ = np.histogram(df_diff["term_normalized"], bins=bin_edges)

    # 累積度数分布を計算
    cumulative_distribution = np.cumsum(term_distribution)

    # 累積度数分布の最大値を取得
    max_cumulative_value = cumulative_distribution[-1]  # 最後の値が最大値

    # 正規化
    if max_cumulative_value != 0:
        normalized_cumulative_distribution = cumulative_distribution / max_cumulative_value
    else:
        normalized_cumulative_distribution = cumulative_distribution  # ゼロの場合は正規化せず

    return normalized_cumulative_distribution, bin_edges


def plot_term_distribution(term_distribution, bin_edges):
    """
    term の度数分布をプロットする
    Args:
        term_distribution: ビンごとの頻度
        bin_edges: ビン境界リスト
    """
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
    plt.figure(figsize=(40/25.4,30/25.4))
    mplib.rcParams['font.size'] = 6
    plt.bar(bin_centers, term_distribution, width=np.diff(bin_edges), edgecolor="black", align="center")
    plt.xlabel("Term Range", color='black')
    plt.ylabel("Frequency", color='black')
    plt.title("Term Frequency Distribution")
    plt.tight_layout()

    save = True
    save_path = ".//"
    save_file_name = "angle_distribution_plot"
    if len(sys.argv) == 3:
        save_file_name = "angle_distribution_from_" + sys.argv[1] + "_to_" + sys.argv[2]
    if save==True:
        plt.savefig(save_path + save_file_name + ".pdf")
        #plt.savefig(save_path + save_file_name + ".svg")

    plt.show()

def plot_cumulative_distribution(normalized_cumulative_distribution, term_distribution, bin_edges, max_y1=1.5, min_y2=0.6):
    """
    累積度数分布をプロットし、データ範囲外で水平線を描画
    Args:
        normalized_cumulative_distribution: 累積度数分布（正規化済み）
        bin_edges: ビン境界リスト
        max_x: X軸の最大値
    """
    mplib.rcParams['font.size'] = 6

    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
    # plt.figure(figsize=(10, 6))

    fig = plt.figure(figsize=(40/25.4,30/25.4))
    # ax1 = fig.add_subplot()
    # plt.gca().set_aspect(1)
    # ax1.yaxis.set_major_formatter(FormatStrFormatter('%.1f'))


    # plt.xticks(np.arange(0, 1, step=10))  # 50刻みの間隔を設定

    
    # ax1.set_facecolor(color='white')

    # ax1.bar(bin_centers, term_distribution/1000, width=np.diff(bin_edges), color='purple', align="center", label="Count")
    # ax1.set_ylim(0, max_y1)  # 累積度数分布のY軸スケールを [0, 1] に設定


    # Y軸（累積度数分布用）を右側に追加
    # ax2 = ax1.twinx()
    ax2 = fig.add_subplot()
    ax2.set_xlabel("Normalized contact force")


    ax2.set_facecolor(color='white')#'#3E3A39'
    ax2.plot(bin_centers, (normalized_cumulative_distribution-min_y2)/(1.0-min_y2), markersize=3, color='purple', label="Normalized cumulative Distribution", linestyle='-', linewidth=2)
    ax2.set_ylim(0, 1.1)  # 累積度数分布のY軸スケールを [0, 1] に設定

    # 水平線を追加（累積度数分布の最大値以降）
    ax2.hlines(
        y=normalized_cumulative_distribution[-1],  # 累積度数分布の最大値（最後の値）
        xmin=bin_centers[-1],                      # 最後のビンの中心
        xmax=1,                                # 水平線を描画するX軸の最大値
        colors='purple',
        linestyles='--',
        linewidth=2,
        label='Horizontal Extension'
    )

    # for spine in ax1.spines.values():
    #     spine.set_edgecolor('black')
    for spine in ax2.spines.values():
        spine.set_edgecolor('black')

    # ax1.tick_params(colors='black')
    ax2.tick_params(colors='black')

    # グリッド線を有効にする
    # ax2.grid(axis='x', linestyle='--', alpha=0.7)
    # ax2.grid(axis='y', linestyle='--', alpha=0.7)
    ax2.tick_params(width = 1, length = 1)
    ax2.grid(True, which='both', linestyle='--', linewidth=0.7, color='gray')
    # ax1.xaxis.set_major_locator(ticker.MultipleLocator(0.25))

    # X軸のスケールを設定
    ax2.set_xlim(0, 1)

    #fig.suptitle("Normalized Cumulative Term Frequency Distribution", color='black')
    # ax1.legend(loc='upper left', labelcolor='black', facecolor='black', edgecolor='black')
    # ax2.legend(loc='upper right', labelcolor='black', facecolor='black', edgecolor='black')
    # ax1.set_xlabel("Contact force", color='black')

    fig.tight_layout()

    # グラフを保存
    save = True
    save_path = ".//"
    save_file_name = "normalized_cumulative_distribution_plot_with_extension"
    if len(sys.argv) == 3:
        save_file_name = "cumulative_distribution_from_" + sys.argv[1] + "_to_" + sys.argv[2]
    if save:
        plt.savefig(save_path + save_file_name + ".pdf")

    plt.show()


def plot_cumulative_distribution_solo(normalized_cumulative_distribution, bin_edges):
    """
    累積度数分布をプロットする
    Args:
        normalized_cumulative_distribution: 累積度数分布（正規化済み）
        bin_edges: ビン境界リスト
    """
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
    plt.figure(figsize=(10, 6))
    plt.plot(bin_centers, normalized_cumulative_distribution, marker='o', linestyle='-', color='blue')
    plt.xlabel("Term Range")
    plt.ylabel("Normalized Cumulative Frequency")
    plt.title("Normalized Cumulative Term Frequency Distribution")
    plt.tight_layout()

    save = True
    save_path = ".//"
    save_file_name = "normalized_cumulative_distribution_plot"
    if len(sys.argv) == 3:
        save_file_name = "cumulative_distribution_from_" + sys.argv[1] + "_to_" + sys.argv[2]
    if save:
        plt.savefig(save_path + save_file_name + ".pdf")
#     plt.show()

def main(n1, n2):
    # データを読み込む
    df_diff, df_base = load_data_with_base(n1, n2, excel_path="out/")

    # 基準データから term_range とビン数を決定
    term_range, div_num = determine_term_range_and_bins(df_base)

    # 度数分布を作成
    term_distribution, bin_edges = create_term_frequency_distribution(df_diff, term_range, div_num)
    # 累積度数分布を作成（正規化済み）
    # normalized_cumulative_distribution, bin_edges, min_bin_info = create_normalized_cumulative_distribution(
    #     df_diff, term_range, div_num
    # )

    normalized_cumulative_distribution, bin_edges = create_normalized_cumulative_distribution(
        df_diff, term_range, div_num
    )


    # 度数分布をプロット
    #plot_term_distribution(term_distribution, bin_edges)
    plot_cumulative_distribution(normalized_cumulative_distribution, term_distribution, bin_edges)

# 実行部分
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