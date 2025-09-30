ST_F(FrameRateTest, GetDisplayFrameRate_Success)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke([](char* framerate) {
            strcpy(framerate, "1920x1080x60");
            return 0;
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_TRUE(response.find("\"success\":true") != string::npos);
    EXPECT_TRUE(response.find("\"framerate\":\"1920x1080x60\"") != string::npos);
}

TEST_F(FrameRateTest, GetDisplayFrameRate_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetFrmMode_Success)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke([](int* frfmode) {
            *frfmode = 1;
            return 0;
        }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_TRUE(response.find("\"success\":true") != string::npos);
    EXPECT_TRUE(response.find("\"auto-frm-mode\":1") != string::npos);
}

TEST_F(FrameRateTest, GetFrmMode_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
}

TEST_F(FrameRateTest, SetCollectionFrequency_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":5000}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetCollectionFrequency_MinimumValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":100}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetCollectionFrequency_InvalidParameter_BelowMinimum)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":50}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_Success)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetDisplayFrameRate_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidParameter_WrongFormat)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"invalid\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidParameter_SingleX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidParameter_NoDigitStart)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"x1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidParameter_NoDigitEnd)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60x\"}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_Success_Mode0)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetFrmMode_Success_Mode1)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetFrmMode_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_InvalidParameter_NegativeValue)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":-1}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_InvalidParameter_ValueTooHigh)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":2}"), response));
}

TEST_F(FrameRateTest, StartFpsCollection_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, StartFpsCollection_AlreadyInProgress)
{
    handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, StopFpsCollection_Success)
{
    handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response);
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, StopFpsCollection_NotStarted)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":60}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_Success_ZeroValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":0}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_InvalidParameter_NegativeValue)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":-1}"), response));
}

TEST_F(FrameRateTest, UpdateFps_HighValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":120}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}
