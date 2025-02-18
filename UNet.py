import fvdb
import torch

class DownSampleBlock(torch.nn.Module):
    def __init__(self, in_channels, out_channels):
        super().__init__()
        self.conv1 = fvdb.nn.SparseConv3d(in_channels=in_channels, out_channels=out_channels, kernel_size=3, stride=1, bias=False)
        self.norm1 = fvdb.nn.BatchNorm(num_features=out_channels)

        self.conv2 = fvdb.nn.SparseConv3d(in_channels=out_channels, out_channels=out_channels, kernel_size=3, stride=1, bias=False)
        self.norm2 = fvdb.nn.BatchNorm(num_features=out_channels)

        self.relu = fvdb.nn.ReLU(inplace=True)
        self.maxpool = fvdb.nn.MaxPool(2, 2)

    def forward(self, x):
        x = self.conv1(x)
        x = self.norm1(x)
        x = self.relu(x)

        x = self.conv2(x)
        x = self.norm2(x)

        return x, self.maxpool(x)
    
class UpSampleBlock(torch.nn.Module):
    def __init__(self, in_channels, out_channels):
        super().__init__()
        self.upconv = fvdb.nn.SparseConv3d(in_channels=in_channels, out_channels=out_channels, transposed=True)
        self.upNorm = fvdb.nn.BatchNorm(out_channels)

        # jcat will double the channel size
        self.conv1 = fvdb.nn.SparseConv3d(in_channels=out_channels*2, out_channels=out_channels, kernel_size=3, stride=1, bias=False)
        self.norm1 = fvdb.nn.BatchNorm(out_channels)

        self.conv2 = fvdb.nn.SparseConv3d(in_channels=out_channels, out_channels=out_channels, kernel_size=3, stride=1, bias=False)
        self.norm2 = fvdb.nn.BatchNorm(out_channels)

        self.relu = fvdb.nn.ReLU(inplace=True)

    def forward(self, x, x_skip):
        x = self.upconv(x, x_skip.grid)
        x = self.upNorm(x)
        x = self.relu(x)
        x = fvdb.jcat([x, x_skip], dim=1)

        x = self.conv1(x)
        x = self.norm1(x)
        x = self.relu(x)

        x = self.conv2(x)
        x = self.norm2(x)
        x = self.relu(x)

        return x
    
class SparseUNet(torch.nn.Module):
    def __init__(self, total_classes):
        super().__init__()

        self.downSampleBlock1 = DownSampleBlock(1, 8)
        self.downSampleBlock2 = DownSampleBlock(8, 16)
        self.downSampleBlock3 = DownSampleBlock(16, 32)
        self.downSampleBlock4 = DownSampleBlock(32, 64)

        self.bottleneck1 = fvdb.nn.SparseConv3d(in_channels=64, out_channels=128, kernel_size=3, stride=1, bias=False)
        self.bottleNorm1 = fvdb.nn.BatchNorm(num_features=128)
        self.bottleneck2 = fvdb.nn.SparseConv3d(in_channels=128, out_channels=128, kernel_size=3, stride=1, bias=False)
        self.bottleNorm2 = fvdb.nn.BatchNorm(128)

        self.upsample4 = UpSampleBlock(128, 64)
        self.upsample3 = UpSampleBlock(64, 32)
        self.upsample2 = UpSampleBlock(32, 16)
        self.upsample1 = UpSampleBlock(16, 8)

        self.preFinalConv = fvdb.nn.SparseConv3d(in_channels=8, out_channels=8, kernel_size=3, stride=1, bias=False)
        self.preFinalNorm = fvdb.nn.BatchNorm(8)

        self.finalConv = fvdb.nn.SparseConv3d(in_channels=8, out_channels=total_classes, kernel_size=3, stride=1, bias=True)

        self.relu = fvdb.nn.ReLU(inplace=True)

    def forward(self, x):
        og1, x = self.downSampleBlock1(x)

        og2, x = self.downSampleBlock2(x)
        og3, x = self.downSampleBlock3(x)
        og4, x = self.downSampleBlock4(x)

        x = self.bottleneck1(x)
        x = self.bottleNorm1(x)
        x = self.bottleneck2(x)
        x = self.bottleNorm2(x)

        x = self.upsample4(x, og4)
        x = self.upsample3(x, og3)
        x = self.upsample2(x, og2)
        x = self.upsample1(x, og1)

        x = self.preFinalConv(x)
        x = self.preFinalNorm(x)
        x = self.finalConv(x)

        return x