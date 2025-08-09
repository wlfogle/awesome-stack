#include "enhanced_rgb_controller.h"

EnhancedRGBController::EnhancedRGBController(const QString &devicePath)
    : devicePath(devicePath) {
    initializeKeyMappings();
}

bool EnhancedRGBController::checkPermissions() const {
    QFileInfo fileInfo(devicePath);
    return fileInfo.exists() && fileInfo.isWritable();
}

bool EnhancedRGBController::sendKeyCommand(int keyIndex, int red, int green, int blue) {
    QFile device(devicePath);
    if (!device.open(QIODevice::WriteOnly))
        return false;

    QByteArray command = QByteArray::fromRawData(
        reinterpret_cast<const char *>(std::array{0xCC, 0x01, keyIndex, red, green, blue, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}.data()), 16);

    device.write(command);
    return true;
}

bool EnhancedRGBController::setKeyColor(const QString &keyName, int red, int green, int blue) {
    int keyIndex = keyboardMap.value(keyName.toLower(), -1);
    if (keyIndex == -1)
        return false;
    return sendKeyCommand(keyIndex, red, green, blue);
}

bool EnhancedRGBController::setGroupColor(const QString &groupName, int red, int green, int blue) {
    if (!keyGroups.contains(groupName))
        return false;
    for (const QString &key : keyGroups[groupName]) {
        setKeyColor(key, red, green, blue);
    }
    return true;
}

bool EnhancedRGBController::clearAllKeys() {
    for (int i = 0; i < 0xFF; ++i) {
        sendKeyCommand(i, 0, 0, 0);
    }
    return true;
}

void EnhancedRGBController::advancedWaveEffect(int duration, const QString &waveType) {
    // Placeholder for advanced wave effect implementation
}

void EnhancedRGBController::initializeKeyMappings() {
    keyboardMap = {
        {"esc", 0x00}, {"f1", 0x01}, {"f2", 0x02}, {"f3", 0x03}, {"f4", 0x04},
        {"f5", 0x05}, {"f6", 0x06}, {"f7", 0x07}, {"f8", 0x08}, {"f9", 0x09},
        {"f10", 0x0A}, {"f11", 0x0B}, {"f12", 0x0C}, {"prtsc", 0x0D}, {"scroll", 0x0E},
        {"pause", 0x0F}, {"home", 0x10}, {"ins", 0x11}, {"pgup", 0x12}, {"pgdn", 0x13},
        {"del", 0x14}, {"end", 0x15}, {"grave", 0x20}, {"1", 0x21}, {"2", 0x22},
        {"3", 0x23}, {"4", 0x24}, {"5", 0x25}, {"6", 0x26}, {"7", 0x27},
        {"8", 0x28}, {"9", 0x29}, {"0", 0x2A}, {"minus", 0x2B}, {"equals", 0x2D},
        {"backspace", 0x2E}, {"numlock", 0x30}, {"kp_divide", 0x31}, {"kp_multiply", 0x32}, {"kp_minus", 0x33},
        {"kp_7", 0x50}, {"kp_8", 0x51}, {"kp_9", 0x52}, {"kp_plus", 0x53}, {"kp_4", 0x70},
        {"kp_5", 0x71}, {"kp_6", 0x72}, {"kp_1", 0x90}, {"kp_2", 0x91}, {"kp_3", 0x92},
        {"kp_enter", 0x93}, {"kp_0", 0xB1}, {"kp_period", 0xB2}, {"tab", 0x40}, {"q", 0x42},
        {"w", 0x43}, {"e", 0x44}, {"r", 0x45}, {"t", 0x46}, {"y", 0x47},
        {"u", 0x48}, {"i", 0x49}, {"o", 0x4A}, {"p", 0x4B}, {"lbracket", 0x4C},
        {"rbracket", 0x4D}, {"backslash", 0x4E}, {"capslock", 0x60}, {"a", 0x62}, {"s", 0x63},
        {"d", 0x64}, {"f", 0x65}, {"g", 0x66}, {"h", 0x67}, {"j", 0x68},
        {"k", 0x69}, {"l", 0x6A}, {"semicolon", 0x6B}, {"quote", 0x6C}, {"enter", 0x6E},
        {"lshift", 0x80}, {"z", 0x83}, {"x", 0x84}, {"c", 0x85}, {"v", 0x86},
        {"b", 0x87}, {"n", 0x88}, {"m", 0x89}, {"comma", 0x8A}, {"period", 0x8B},
        {"slash", 0x8C}, {"rshift", 0x8D}, {"up", 0x8F}, {"left", 0xAE}, {"down", 0xAF},
        {"right", 0xB0}, {"lctrl", 0xA0}, {"fn", 0xA2}, {"lalt", 0xA4}, {"space", 0xA8},
        {"ralt", 0xAA}, {"menu", 0xAB}, {"rctrl", 0xAC}
    };

    keyGroups = {
        {"function_keys", {"f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12"}},
        {"number_row", {"grave", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "minus", "equals"}},
        {"qwerty_row", {"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "lbracket", "rbracket", "backslash"}},
        {"asdf_row", {"capslock", "a", "s", "d", "f", "g", "h", "j", "k", "l", "semicolon", "quote", "enter"}},
        {"zxcv_row", {"lshift", "z", "x", "c", "v", "b", "n", "m", "comma", "period", "slash", "rshift"}},
        {"bottom_row", {"lctrl", "fn", "lalt", "space", "ralt", "menu", "rctrl"}},
        {"arrow_keys", {"up", "left", "down", "right"}},
        {"keypad", {"numlock", "kp_divide", "kp_multiply", "kp_minus", "kp_7", "kp_8", "kp_9", "kp_plus",
                     "kp_4", "kp_5", "kp_6", "kp_1", "kp_2", "kp_3", "kp_enter", "kp_0", "kp_period"}},
    };
}

std::tuple<int, int, int> EnhancedRGBController::hsvToRgb(float h, float s, float v) const {
    float r, g, b;
    int i = static_cast<int>(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch(i % 6){
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
    }

    return std::make_tuple(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255));
}
