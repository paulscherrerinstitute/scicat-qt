pragma Singleton

import QtQuick

// Colours by role, spacing and type sizes. Every colour has a light and a dark value; pairs used for text keep 4.5:1.
QtObject {
    readonly property bool dark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    // brand frame, the same in both themes
    readonly property color brand: "#002D3B"
    readonly property color brandRaised: "#0B4A5E"
    readonly property color brandHover: "#07394A"
    readonly property color brandText: "#FFFFFF"
    readonly property color brandMuted: "#9FB6BD"
    readonly property color brandMarker: "#27AAE1"

    readonly property color accent: dark ? "#0E7CAD" : "#0B6E99"
    readonly property color link: dark ? "#7CCBF0" : "#0B6E99"
    readonly property color focus: dark ? "#7CCBF0" : "#0B6E99"

    readonly property color window: dark ? "#14181A" : "#F4F6F7"
    readonly property color surface: dark ? "#1D2225" : "#FFFFFF"
    readonly property color surfaceAlt: dark ? "#252B2F" : "#EEF1F3"
    readonly property color hover: dark ? "#2A3135" : "#E8ECEE"
    readonly property color selected: dark ? "#123A4A" : "#DDF0F9"
    readonly property color outline: dark ? "#3A4348" : "#D5DADD"
    readonly property color outlineStrong: dark ? "#6A757B" : "#869096"

    readonly property color text: dark ? "#E8ECEE" : "#1B2124"
    readonly property color textSecondary: dark ? "#A9B1B5" : "#5B6468"
    readonly property color textDisabled: dark ? "#6A757B" : "#9AA3A8"
    readonly property color textOnAccent: "#FFFFFF"
    readonly property color danger: dark ? "#C2413A" : "#B3261E"

    function toneText(tone: string): color {
        switch (tone) {
        case "success": return dark ? "#8FE0AB" : "#1B6E3C"
        case "warning": return dark ? "#F5C680" : "#7A4A00"
        case "error": return dark ? "#F5B1AB" : "#9C1F18"
        case "info": return dark ? "#93D4F3" : "#0A5F85"
        case "qa": return dark ? "#F5C680" : "#5C3B00"
        case "development": return dark ? "#D3C2F7" : "#3F2A6B"
        case "local": return dark ? "#A6E3B6" : "#1F4D2B"
        }
        return dark ? "#C8CED1" : "#454C50"
    }

    function toneFill(tone: string): color {
        switch (tone) {
        case "success": return dark ? "#173D27" : "#E3F4E8"
        case "warning": return dark ? "#47330F" : "#FCEFD6"
        case "error": return dark ? "#4D1B18" : "#FBE4E2"
        case "info": return dark ? "#11394C" : "#DDF0F9"
        case "qa": return dark ? "#47330F" : "#FFD98A"
        case "development": return dark ? "#33264F" : "#E2D6FA"
        case "local": return dark ? "#183D23" : "#CFEFD8"
        }
        return dark ? "#33383B" : "#E9ECEE"
    }

    function toneIcon(tone: string): string {
        switch (tone) {
        case "success": return "checkCircle"
        case "warning": return "alert"
        case "error": return "error"
        }
        return "info"
    }

    readonly property int gapXs: 4
    readonly property int gapS: 8
    readonly property int gapM: 12
    readonly property int gapL: 16
    readonly property int gapXl: 24
    readonly property int gapXxl: 32
    readonly property int radius: 8
    readonly property int radiusSmall: 6

    // modular scale, ratio 1.2 on a 14 px body
    readonly property int fontSmall: 12
    readonly property int fontBody: 14
    readonly property int fontTitle: 17
    readonly property int fontHeading: 20
    readonly property int fontDisplay: 24
    readonly property string monoFamily: Qt.platform.os === "osx" ? "Menlo" : Qt.platform.os === "windows" ? "Consolas" : "DejaVu Sans Mono"

    readonly property int rowHeight: 44
    readonly property int formWidth: 560
}
