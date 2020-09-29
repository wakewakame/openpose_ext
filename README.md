# 使用方法
## 準備
### Visual Studio Community 2019のインストール
既にインストールされている場合はこの手順は飛ばしてください。  
以下のURLより、Visual Studio Community 2019をダウンロード、インストールします。  
[https://visualstudio.microsoft.com/ja/downloads/](https://visualstudio.microsoft.com/ja/downloads/)  
### gitのインストール
既にインストールされている場合はこの手順は飛ばしてください。  
まず、Visual Studio Installerを起動します。  
スタートメニューから`isual Studio Installer`と入力すると見つかると思います。  
起動すると、`インストール済み`の中に`Visual Studio Community 2019`があります。  
もしも`Visual Studio Community 2019`に`更新`ボタンがあれば、それを押して更新を行います。  
`Visual Studio Community 2019`の`変更`ボタンを押し、`個別のコンポーネント`を押します。  
その下の一覧から`Git for Windows`にチェックし、右下の`変更`を押します。 
### CMakeのインストール
既にインストールされている場合はこの手順は飛ばしてください。  
以下のURLより`cmake-x.x.x-win64-x64.msi`(x.x.xはバージョン)をダウンロード、インストールします。  
[https://cmake.org/download/](https://cmake.org/download/)  
注意点として、インストーラーの途中に`Choose options for installing CMake`という選択画面があるので`Add CMake to the system PATH for all users`を選択します。  
### CUDAとcuDNNのインストール
既にインストールされている場合はこの手順は飛ばしてください。  
以下のURLよりCUDA 10.1をダウンロード、インストールします。  
[https://developer.nvidia.com/cuda-10.1-download-archive-base](https://developer.nvidia.com/cuda-10.1-download-archive-base)  
次に、CUDA 10.1に対応するcuDNNをインストールします。  
以下のURLよりダウンロード、インストールしてください。(アカウント登録が必要です)  
[https://developer.nvidia.com/rdp/cudnn-download](https://developer.nvidia.com/rdp/cudnn-download)  
## 環境構築
エクスプローラーでopenpose_extをダウンロードしたい場所を開きます。  
次に、エクスプローラーのアドレスバーに`cmd`と入力します。  
コマンドプロンプトが起動するので、`git clone https://github.com/wakewakame/openpose_ext`と入力し、エンターを押します。  
これでopenpose_extのプログラムがダウンロードされます。  
次に、コマンドプロンプトで以下のコマンドを続けて入力します  
`cd openpose_ext` (openpose_extフォルダに移動)  
`mkdir build` (buildフォルダの生成)  
`cd build` (buildフォルダに移動)  
`cmake ..` (openpose_extフォルダに対してCMakeを実行)  
この`cmake ..`の処理には数十分から数時間ほどかかることがあります。  
以上の処理が成功すると、openpose_extフォルダ内のbuildフォルダに`openpose_ext.sln`が生成されます。  
## ビルド
openpose_extフォルダ内のbuildフォルダに生成された`openpose_ext.sln`を開きます。  
画面上部の`ローカル Windows デバッガー`を押し、ビルドを開始します。  
ビルドが終わると、openpose_extが起動します。  
動画ファイルを変更したい場合は、`openpose_ext/main.cpp`の`media/video.mp4`の箇所を適宜変更してください。  
## サンプルプログラム
`openpose_ext/examples`にサンプルプログラムがいくつか準備されています。  