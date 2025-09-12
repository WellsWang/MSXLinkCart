# MSXLinkCart

Build a Simple Flashable Cartridge for MSX computer.

创建一个简单的可以烧录的MSX游戏卡带。

<img width="1552" height="1000" alt="cover" src="https://github.com/user-attachments/assets/65bdb2c4-a828-4878-ae80-e9e8c87b4f1c" />
<img width="1920" height="1080" alt="DSC_1268 MP4_20250912_145425 868" src="https://github.com/user-attachments/assets/b6dccf27-ecfd-4d3a-81ff-c4f038ed8992" />


## 文件说明
 - CartridgeShell  卡带外壳相关
   - CartCoverArt  卡带封面图片
     - cart_label_back.png  后盖贴纸图案
     - cart_label_front.png  上盖贴纸图案
   - MSXLINKCart Shell Bottom.*  外壳后盖三维模型
   - MSXLINKCart Shell Top.*  外壳上盖三维模型  
 - PCB  电路设计相关
   - BOM_MSXLinkCart.V1.csv  BOM清单
   - Schematics.pdf 电路原理图
   - Gerber_MSXLinkCart.V1.zip  PCB制作Gerbers文件
   - MSXLinkCart.V1.epro  嘉立创工程文件
 - src  源代码相关
   - app  PC端应用程序
     - msxlinkcart.py  PC端烧录工具程序 
   - firmware  固件
     - MSXLINKCart/MSXLINKCart.ino  固件源码  
 - LICENSE  许可证
 - README.md  本说明文件

## 使用方法

### PCB及硬件制作
使用Gerbers文件制作PCB后，按照BOM文件采购元件，并仔细完成焊接，确保每个引脚被可靠的焊接在PCB上。

需要注意的是B0512S-1WR3型DC-DC 5V-12V电压转换器不要直接紧贴着焊接在PCB板上，否则无法安装外壳。需要留长一些引脚焊接，焊接后弯折90度。如下图所示。
<img width="960" height="540" alt="DSC_1138 MP4_20250912_151207 605" src="https://github.com/user-attachments/assets/78fe30bd-88a1-4157-bf5e-f99c5d3c55d8" />

### 固件
固件程序需要用Arduino IDE进行编译上传，编译时注意目标芯片为 ATMEGA328P。推荐选择开发板为 `MiniCore` > `ATmega328`。如下图所示。
<img width="739" height="612" alt="IDE" src="https://github.com/user-attachments/assets/f2efc5ff-3255-436c-b5ad-9c017d341130" />

### PC端烧录工具
使用前需要用pip安装所需要的库，然后直接运行即可。

### 使用方式
#### 上传固件
将卡从MSX主机上拔下，用Micro USB线连接电脑，这时卡带显示屏会显示“Waiting for connection...”。然后，在电脑上运行PC端烧录工具。
<img width="960" height="540" alt="DSC_1282 MP4_20250912_145330 784" src="https://github.com/user-attachments/assets/c454c5d1-cdf2-44df-944b-50b5c4b0fe15" />

如果是Linux主机，需要先获取USB串口的可读写权限，例如：`sudo chmod 777 /dev/ttyUSB0`，然后再运行烧录工具程序；Windows主机可以直接运行。

选择ROM文件后，可以修改要在卡带OLED屏幕上显示的游戏名称，然后选择正确的USB串口端口号后点击`烧录`按钮，即可完成烧录。

卡带的显示屏会显示烧录进度。

烧录完成后，拔除USB线。

#### 使用卡带
烧录完成后，将卡带插入MSX主机的卡槽，开机即可使用。

烧录后第一次使用时，默认将从第一个ROM启动，卡带的显示屏会显示当前ROM的信息。

如果需要切换ROM在卡带上电状态按下切换按钮，然后关机再开机即可从另一个ROM启动。卡带会保存从哪个ROM启动的信息，即使关机或拔下卡带也会保存这一信息。
<img width="960" height="540" alt="DSC_1303 MP4_20250912_145507 552" src="https://github.com/user-attachments/assets/4cc5e972-ecf6-4692-b6fb-5fe81c49521c" />

## 限制
由于使用的内存映射机制为最简单的直接映射，因此待烧录的ROM大小不能超过32KB。

作者并未穷尽测试所有ROM，因此不能确认所有符合要求的ROM都能正常运行。

## 免责协议
该项目为尝试性验证项目，代码的有效性和可靠性将不作任何的保证。作者不承担此项目带来的任何直接或间接责任。

您使用的ROM必须是合法获取的有效的MSX软件ROM。

## 许可证
本项目基于GNU GPLv3许可证发布，详细信息请参考LICENSE文件。
