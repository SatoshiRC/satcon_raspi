# satcon_raspi — Raspberry Pi GPIO割り込みでICM-20948を駆動

このリポジトリは、Raspberry Pi上でICM-20948（IMU）をI2C接続し、IMUのINTピン（データレディ割り込み）をGPIO割り込みで受けてサンプリングする最小例を含みます。C++（libgpiod）で割り込みを扱います。

## 接続
- I2C: SDA/SCL を `/dev/i2c-1`（Raspberry Pi標準）へ
- INTピン: ICM-20948の INT を Raspberry Pi の BCM GPIO 17（例）へ接続
  - 必要に応じてプルアップ/プルダウンを用意してください（外付け抵抗推奨）。

`satcon_raspi.cpp` 冒頭で使用GPIOは変更できます:
```cpp
const int INT_GPIO_BCM = 17; // 変更可
```

## 依存関係のインストール（Raspberry Pi 側）
```bash
sudo apt update
sudo apt install -y g++ cmake libgpiod-dev i2c-tools
# I2Cを有効化（未設定の場合）
sudo raspi-config  # Interface Options -> I2C を有効化
```

## ビルド
```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j
```

## 実行
```bash
# 実行にroot権限が必要な場合があります（GPIOアクセスのため）
sudo ./satcon_raspi
```

起動すると WHO_AM_I をチェックし、IMUを初期化した後、GPIO 17 の立ち上がりエッジを待ち受けます。ICM-20948のデータレディ割り込み（RAW_DATA_0_RDY）でコールバックが呼ばれ、最新の加速度・角速度を表示します。

## メモ
- libgpiod v1 系 API を利用しています（ヘッダ `<gpiod.h>`）。Bookworm/Bullseye 標準パッケージで動作します。
- INTピンの出力極性やラッチ/パルスなどは `intPinConfig` の値で調整できます。必要に応じてデータシートに合わせて変更してください。
- `ICM20948_USER.*` はSTM32 HAL向けのコードを含むため本例では使用していません。
