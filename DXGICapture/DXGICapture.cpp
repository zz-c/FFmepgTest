//代码参考 https://www.jianshu.com/p/e775b0f45376
#include <iostream>
#include <d3d11.h>
#include <dxgi1_2.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// 这里一定要是二级指针噢，否则外层是不能获取到对象的
HRESULT initDXGIResources(ID3D11Device** device, ID3D11DeviceContext** deviceContext)
{
    HRESULT ret = S_OK;

    D3D_DRIVER_TYPE DriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };

    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    D3D_FEATURE_LEVEL FeatureLevel;

    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
    {
        ret = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels, D3D11_SDK_VERSION, device, &FeatureLevel, deviceContext);
        if (ret >= 0)
        {
            break;
        }
    }
    if (ret < 0)
    {
        return S_FALSE;
    }
    return S_OK;
}
//为了得到IDXGIOutputDuplication对象，需要进行一连串的QueryInterface操作，这里我还有些地方没有弄懂，只知道是这样的固定操作
// 考验指针功底的时候来了，为什么device是一级指针，dupl是二级指针。(提示：device是传入参数，dupl是传出参数)
// output代指是哪个屏幕，大部分情况下都为0，即只有一块屏幕。如果存在扩展屏，那么output可以为1，2，...
HRESULT initDuplication(ID3D11Device* device, IDXGIOutputDuplication** dupl, UINT output)
{
    IDXGIDevice* DxgiDevice = nullptr;
    HRESULT ret = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
    if (ret < 0)
    {
        return S_FALSE;
    }

    IDXGIAdapter* DxgiAdapter = nullptr;
    ret = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
    DxgiDevice->Release();
    DxgiDevice = nullptr;
    if (ret < 0)
    {
        return S_FALSE;
    }

    IDXGIOutput* DxgiOutput = nullptr;
    ret = DxgiAdapter->EnumOutputs(output, &DxgiOutput);
    DxgiAdapter->Release();
    DxgiAdapter = nullptr;
    if (ret < 0)
    {
        return S_FALSE;
    }

    IDXGIOutput1* DxgiOutput1 = nullptr;
    ret = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
    DxgiOutput->Release();
    DxgiOutput = nullptr;
    if (ret < 0)
    {
        return S_FALSE;
    }

    ret = DxgiOutput1->DuplicateOutput(device, dupl);
    DxgiOutput1->Release();
    DxgiOutput1 = nullptr;
    if (ret < 0)
    {
        return S_FALSE;
    }
    return S_OK;
}

void clearDuplication(IDXGIOutputDuplication* dupl)
{
    if (dupl)
    {
        dupl->Release();
        dupl = nullptr;
    }
}

void clearDXGIResources(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    if (device)
    {
        device->Release();
        device = nullptr;
    }

    if (deviceContext)
    {
        deviceContext->Release();
        deviceContext = nullptr;
    }
}
//AcquireNextFrame()获取图像纹理
// dupl为上面获取的对象传进来即可。frameInfo用来存放相关帧信息。texture2D为映射的图像数据。
HRESULT getFrame(IDXGIOutputDuplication* dupl, DXGI_OUTDUPL_FRAME_INFO* frameInfo, ID3D11Texture2D** texture2D, bool* timeOut)
{
    IDXGIResource* desktopResource;
    // 超时时间为500毫秒
    HRESULT ret = dupl->AcquireNextFrame(500, frameInfo, &desktopResource);
    // 当画面变化较慢时可能会超时，超时是正常现象
    if (ret == DXGI_ERROR_WAIT_TIMEOUT)
    {
        *timeOut = true;
        return S_OK;
    }
    *timeOut = false;
    if (ret < 0)
    {
        return S_FALSE;
    }
    if (texture2D && *texture2D)
    {
        (*texture2D)->Release();
        (*texture2D) = nullptr;
    }
    ret = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(texture2D));
    desktopResource->Release();
    desktopResource = nullptr;
    if (ret < 0)
    {
        return S_FALSE;
    }
    return S_OK;
}

// 每次处理完桌面图像后都需要进行调用
HRESULT doneWithFrame(IDXGIOutputDuplication* dupl)
{
    HRESULT ret = dupl->ReleaseFrame();
    if (ret < 0)
    {
        return S_FALSE;
    }
    return S_OK;
}

void saveBMPFile(const char* filename, void* pBmp, int width, int height)
{
    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == NULL)
    {
        return;
    }
    // 已写入字节数
    DWORD bytesWritten = 0;
    // 位图大小
    int bmpSize = width * height * 4;

    // 文件头
    BITMAPFILEHEADER bmpHeader;
    // 文件总大小 = 文件头 + 位图信息头 + 位图数据
    bmpHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpSize;
    // 固定
    bmpHeader.bfType = 0x4D42;
    // 数据偏移，即位图数据所在位置
    bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    // 保留为0
    bmpHeader.bfReserved1 = 0;
    // 保留为0
    bmpHeader.bfReserved2 = 0;
    // 写文件头
    WriteFile(hFile, (LPSTR)&bmpHeader, sizeof(bmpHeader), &bytesWritten, NULL);

    // 位图信息头
    BITMAPINFOHEADER bmiHeader;
    // 位图信息头大小
    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    // 位图像素宽度
    bmiHeader.biWidth = width;
    // 位图像素高度
    bmiHeader.biHeight = -height;
    // 必须为1
    bmiHeader.biPlanes = 1;
    // 像素所占位数
    bmiHeader.biBitCount = 32;
    // 0表示不压缩
    bmiHeader.biCompression = 0;
    // 位图数据大小
    bmiHeader.biSizeImage = bmpSize;
    // 水平分辨率(像素/米)
    bmiHeader.biXPelsPerMeter = 0;
    // 垂直分辨率(像素/米)
    bmiHeader.biYPelsPerMeter = 0;
    // 使用的颜色，0为使用全部颜色
    bmiHeader.biClrUsed = 0;
    // 重要的颜色数，0为所有颜色都重要
    bmiHeader.biClrImportant = 0;

    // 写位图信息头
    WriteFile(hFile, (LPSTR)&bmiHeader, sizeof(bmiHeader), &bytesWritten, NULL);
    // 写位图数据
    WriteFile(hFile, pBmp, bmpSize, &bytesWritten, NULL);
    CloseHandle(hFile);
}

// data为传入的图像数据，比如acquiredDesktopImage
void saveDesktopImage(ID3D11Texture2D* data, ID3D11Device* device, ID3D11DeviceContext* deviceContext, const char* pictureName)
{
    D3D11_TEXTURE2D_DESC dataDesc;
    // 获取纹理(图像)的相关信息
    data->GetDesc(&dataDesc);

    ID3D11Texture2D* copyDesktop = nullptr;
    dataDesc.Usage = D3D11_USAGE_STAGING;
    dataDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    dataDesc.BindFlags = 0;
    dataDesc.MiscFlags = 0;
    dataDesc.MipLevels = 1;
    dataDesc.ArraySize = 1;
    dataDesc.SampleDesc.Count = 1;

    device->CreateTexture2D(&dataDesc, NULL, &copyDesktop);
    deviceContext->CopyResource(copyDesktop, data);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    deviceContext->Map(copyDesktop, 0, D3D11_MAP_READ, 0, &mappedResource);

    // 保存为bmp图像
    // mappedResource.pData为数据指针
    // 注意dataDesc.Width虽然为图像像素宽度，但是系统为了对齐操作，比如当你的宽不能被4整除时，
    // 系统会填充数据使其宽能够被4整除。而实际数据的宽是多少存放在mappedResource.RowPitch字段中。
    // 请注意此处的区别，否则在后续的分配空间中可能导致内存溢出，这里我没有进行处理，但在实际项目中请注意这个问题。
    saveBMPFile(pictureName, mappedResource.pData, dataDesc.Width, dataDesc.Height);

    copyDesktop->Release();
}

int main()
{

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* deviceContext = nullptr;
    initDXGIResources(&device, &deviceContext);

    IDXGIOutputDuplication* desktopDupl = nullptr;
    UINT output = 0;
    initDuplication(device, &desktopDupl, output);


    DXGI_OUTDUPL_FRAME_INFO frameInfo;

    // 存放桌面图像
    ID3D11Texture2D* acquiredDesktopImage = nullptr;
    bool timeOut;
    int i = 0;
    while (++i < 10)
    {
        HRESULT ret = getFrame(desktopDupl, &frameInfo, &acquiredDesktopImage, &timeOut);
        if (timeOut)
        {
            continue;
        }
        if (ret < 0)
        {
            continue;
        }
        char pictureName[64] = { 0 };
        sprintf_s(pictureName, "desktop%d.bmp", i);
        Sleep(500);
        // 处理图像数据
        saveDesktopImage(acquiredDesktopImage, device, deviceContext, pictureName);
        doneWithFrame(desktopDupl);
        if (acquiredDesktopImage)
        {
            acquiredDesktopImage->Release();
            acquiredDesktopImage = nullptr;
        }
    }

    clearDuplication(desktopDupl);
    clearDXGIResources(device, deviceContext);
    std::cout << "Hello DXGI!\n";
    return 0;
}

