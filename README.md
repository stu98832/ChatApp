# ChatApp

用 C++ 寫的簡易 Command 界面聊天程式



## 功能

* 一對一聊天
* 語音對話



## 如何建置

* 使用 project/vs2017 裡的專案
  1. 開啟專案
  2. 選擇 x86 Debug
  3. 建置
* 自己新增專案
  1. 選擇 x86 架構，使用 Debug 或 Release 皆可
  2. 將 src 內的原始檔加入專案
  3. 增加 project/includes 資料夾作為 #include 的資料來源
  4. 增加 project/libs y 資料夾作為程式庫的資料來源
  5. 連接器引入程式庫 portaudio_x86.lib
  6. 進行建置
  7. 將 dlls 裡的 portaudio_x86.dll 放入程式所在位置即可執行



## 功能

* Tab
    重新調整畫面大小
* Up
    往前看訊息紀錄
* Down
    往後看訊息紀錄
* Left
    游標往左一格
* Right
    游標往右一格
* Home
    游標移到開頭
* End
    游標移到尾巴
* Ctrl+C
    關閉程式
* Ctrl+R
    開關麥克風
* Backspace
    游標往左一格並刪除文字
* Delete
    刪除游標處的文字
* Enter
    發送文字
