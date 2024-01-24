# This is a sample Python script.

# Press Shift+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('TkAgg')


from socket import *
from time import ctime
import numpy as np
import scipy.signal
import random
from matplotlib.pyplot import MultipleLocator
import csv
import string

x_list = []     # 用于存放x轴数据
y_list = []     # 用于存放y轴数据
temp_list_x = []    # 临时存放x轴数据
temp_list_y = []    # 临时存放y轴数据
show_num = 10000   # x轴显示的数据个数，例：show_num = 10表示x轴只显示10个数据
num = 0
plt.ion()	# 打开交互模式
fig1 = plt.figure(figsize=(50,40))             # 设置图片大小

x = 1
# plt.xlim(0, 100)	                        # 设置x轴的数值显示范围
# x_major_locator=MultipleLocator(10)	            # 把x轴的刻度间隔设置为2
# y_major_locator=MultipleLocator(10)		        # 把y轴的刻度间隔设置为10
# ax=plt.gca()	                                # ax为两条坐标轴的实例
# ax.xaxis.set_major_locator(x_major_locator)	    # 把x轴的主刻度设置为2的倍数
# ax.yaxis.set_major_locator(y_major_locator)	    # 把y轴的主刻度设置为10的倍数

def filter_func(rx):
    for i in range(1, len(rx)-2):
        if rx[i - 1] - rx[i] > 20 and rx[i + 1] - rx[i] > 20:
            rx[i] =int( (rx[i - 1] + rx[i + 1] )/ 2)
            continue
        if rx[i-1] - rx[i] > 20 and abs(rx[i+1] - rx[i]) < 50 and rx[i+2] - rx[i+1] > 20:
            rx[i] = int((rx[i -1] + rx[i+2])/2)
            rx[i+1] = rx[i]
            continue
    return rx

def filter_func2(rx):

    for j in range(50):
        m = np.mean(rx[j*20 : j*20 + 20])
        sd = np.std(rx[j*20 : j*20 + 20])
        for index, i in enumerate(rx[j*20 : j*20 + 20]):
            z = (i - m)/sd
            if np.abs(z) > 2:
                rx[j * 20 + index] = int(m)
    return rx



# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    CATCH_NUM = 1000
    catch_counter = 0
    HOST = ''
    POST = 8001
    BUFSIZ = 10
    ADDR = (HOST, POST)
    dataprev = 1
    rev_buf = []
    rssi_buf = []
    note = open(r'/home/lodhi/Desktop/openvlc/data/labeltxt/rawdata.txt', mode = 'a')
    try:
        tcpSerSock = socket(AF_INET, SOCK_STREAM)
        tcpSerSock.bind(ADDR)
        tcpSerSock.listen(5)

        while True:
            print('waiting for connecting...')
            tcpCliSock, addr = tcpSerSock.accept()
            print('...connecting from:', addr)
            while True:
                data = tcpCliSock.recv(BUFSIZ)
                if not data:
                    continue
                if len(data) < 9:
                    print(data)
                else:
                    rev_buf.append(data)
                if len(rev_buf) == CATCH_NUM:
                    catch_counter = catch_counter + 1
                    print('receive ', catch_counter, ' data')
                    if num > show_num:
                        plt.cla()
                        plt.xlim(0, 11000)
                        plt.ylim(0, 800)
                        ax = plt.gca()
                        num = 0
                    for i in range(0, CATCH_NUM):
                        # print(rev_buf[i].decode())
                        try:
                            rssi_buf.append(int(rev_buf[i].decode().split('\r\n')[0]))
                        except:
                            print('error int!!!')
                            rssi_buf.append(int(rev_buf[i - 5].decode().split('\r\n')[0]))
                    # rssi_buf_f = filter_func2(rssi_buf)
                    # rssi_buf_f2 = filter_func(rssi_buf_f)

                    rssi_buf_save = [str(line) + '\n' for line in rssi_buf]
                    note.write('label-----' + str(catch_counter) + '\n')
                    note.writelines(rssi_buf_save)
                    plt.plot(range(num, num + 1000) , rssi_buf)
                    plt.pause(0.01)
                    # plt.show()
                    # for i in range(0, CATCH_NUM):
                    #     print(rssi_buf[i])
                    num = num + 1000
                    rev_buf.clear()
                    rssi_buf.clear()
            tcpCliSock.close()
    except Exception as e:
        print(e)
    tcpSerSock.close()


    # i = 0
    # inputpath = '/home/lodhi/Desktop/openvlc/data/output4.txt'
    # with open(inputpath, 'r', encoding = 'utf-8') as file:
    #     data = []
    #     for line in file:
    #         data_line = line.strip("\n").split()
    #         print(data_line)
    #         if len(data_line[0]) > 4 or data_line[0][0] == '\\':
    #             continue
    #         data.append([int(i) for i in data_line])
    #         i = i + 1
    #         print(i)
    #     print(data)
    #     print(i)
    # plt.plot(data)
    # inputpath = '/home/lodhi/Desktop/openvlc/data/output9.txt'
    # with open(inputpath, 'r', encoding = 'utf-8') as file:
    #     data = []
    #     for line in file:
    #         data_line = line.strip("\n").split()
    #         print(data_line)
    #         if len(data_line[0]) > 4 or data_line[0][0] == '\\':
    #             continue
    #         data.append([int(i) for i in data_line])
    #         i = i + 1
    #         print(i)
    #     print(data)
    #     print(i)
    # plt.plot(data)
    # inputpath = '/home/lodhi/Desktop/openvlc/data/output5.txt'
    # with open(inputpath, 'r', encoding = 'utf-8') as file:
    #     data = []
    #     for line in file:
    #         data_line = line.strip("\n").split()
    #         print(data_line)
    #         if len(data_line[0]) > 4 or data_line[0][0] == '\\':
    #             continue
    #         data.append([int(i) for i in data_line])
    #         i = i + 1
    #         print(i)
    #     print(data)
    #     print(i)
    # plt.plot(data)
    # inputpath = '/home/lodhi/Desktop/openvlc/data/output7.txt'
    # with open(inputpath, 'r', encoding = 'utf-8') as file:
    #     data = []
    #     for line in file:
    #         data_line = line.strip("\n").split()
    #         print(data_line)
    #         if len(data_line[0]) > 4 or data_line[0][0] == '\\':
    #             continue
    #         data.append([int(i) for i in data_line])
    #         i = i + 1
    #         print(i)
    #     print(data)
    #     print(i)
    # # plt.plot(data)
    # plt.show()
# See PyCharm help at https://www.jetbrains.com/help/pycharm/
