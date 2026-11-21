pragma Singleton

import QtQuick

// Line icons on a 24 x 24 grid, drawn with a 2 px round stroke by Icon.
QtObject {
    readonly property string ring: "M3 12 a9 9 0 1 0 18 0 a9 9 0 1 0 -18 0"
    readonly property var paths: ({
        "home": "M4 11 L12 4 L20 11 M6 10 V20 H18 V10",
        "upload": "M4 15 v3 a2 2 0 0 0 2 2 h12 a2 2 0 0 0 2 -2 v-3 M12 16 V4 M7 9 L12 4 L17 9",
        "download": "M4 15 v3 a2 2 0 0 0 2 2 h12 a2 2 0 0 0 2 -2 v-3 M12 4 V16 M7 11 L12 16 L17 11",
        "activity": "M3 12 h4 l3 -7 l4 14 l3 -7 h4",
        "settings": "M4 6 h10 M18 6 h2 M4 12 h4 M12 12 h8 M4 18 h12 M14 6 a2 2 0 1 0 4 0 a2 2 0 1 0 -4 0 M8 12 a2 2 0 1 0 4 0 a2 2 0 1 0 -4 0 M16 18 a2 2 0 1 0 4 0 a2 2 0 1 0 -4 0",
        "help": ring + " M9.5 9.5 a2.5 2.5 0 1 1 3.5 2.3 c-0.7 0.4 -1 0.9 -1 1.7 M12 16.5 v0.5",
        "user": "M8 8 a4 4 0 1 0 8 0 a4 4 0 1 0 -8 0 M4 20 c0 -4 4 -6 8 -6 s8 2 8 6",
        "people": "M5 8.5 a3.5 3.5 0 1 0 7 0 a3.5 3.5 0 1 0 -7 0 M2.5 19.5 c0 -3.5 3 -5.5 6 -5.5 s6 2 6 5.5 M14.5 5.3 a3.5 3.5 0 0 1 0 6.4 M17 14.3 c2.5 0.6 4.5 2.3 4.5 5.2",
        "logout": "M9 4 H6 a2 2 0 0 0 -2 2 v12 a2 2 0 0 0 2 2 h3 M16 8 l4 4 l-4 4 M20 12 H9",
        "search": "M4 11 a7 7 0 1 0 14 0 a7 7 0 1 0 -14 0 M16.5 16.5 L21 21",
        "refresh": "M4 12 a8 8 0 0 1 14.9 -4 M20 4 v4 h-4 M20 12 a8 8 0 0 1 -14.9 4 M4 20 v-4 h4",
        "check": "M5 12.5 l4.5 4.5 L19 7.5",
        "checkCircle": ring + " M8 12.5 l3 3 l5 -6",
        "alert": "M12 4 L21 19.5 H3 Z M12 10 v4 M12 17 v0.5",
        "error": ring + " M12 7.5 v5.5 M12 16 v0.5",
        "info": ring + " M12 11 v5.5 M12 7.5 v0.5",
        "folder": "M3 7 a2 2 0 0 1 2 -2 h4 l2 2 h8 a2 2 0 0 1 2 2 v8 a2 2 0 0 1 -2 2 H5 a2 2 0 0 1 -2 -2 Z",
        "file": "M7 3 h7 l5 5 v11 a2 2 0 0 1 -2 2 H7 a2 2 0 0 1 -2 -2 V5 a2 2 0 0 1 2 -2 Z M14 3 v5 h5",
        "close": "M6 6 L18 18 M18 6 L6 18",
        "plus": "M12 5 v14 M5 12 h14",
        "minus": "M5 12 h14",
        "trash": "M4 7 h16 M9 7 V4.5 h6 V7 M6.5 7 l1 12.5 h9 l1 -12.5 M10 11 v5 M14 11 v5",
        "copy": "M9 9 h10 a1 1 0 0 1 1 1 v10 a1 1 0 0 1 -1 1 H9 a1 1 0 0 1 -1 -1 V10 a1 1 0 0 1 1 -1 Z M5 15 H4.5 a1 1 0 0 1 -1 -1 V4.5 a1 1 0 0 1 1 -1 H14 a1 1 0 0 1 1 1 V5",
        "chevronRight": "M9 6 l6 6 l-6 6",
        "chevronLeft": "M15 6 l-6 6 l6 6",
        "chevronDown": "M6 9 l6 6 l6 -6",
        "chevronUp": "M6 15 l6 -6 l6 6",
        "arrowUp": "M12 19 V5 M6 11 l6 -6 l6 6",
        "arrowDown": "M12 5 v14 M6 13 l6 6 l6 -6",
        "external": "M14 4 h6 v6 M20 4 L11 13 M18 14 v4 a2 2 0 0 1 -2 2 H6 a2 2 0 0 1 -2 -2 V8 a2 2 0 0 1 2 -2 h4",
        "eye": "M2 12 c3 -5 6.5 -7 10 -7 s7 2 10 7 c-3 5 -6.5 7 -10 7 s-7 -2 -10 -7 Z M9 12 a3 3 0 1 0 6 0 a3 3 0 1 0 -6 0",
        "eyeOff": "M2 12 c3 -5 6.5 -7 10 -7 s7 2 10 7 c-3 5 -6.5 7 -10 7 s-7 -2 -10 -7 Z M9 12 a3 3 0 1 0 6 0 a3 3 0 1 0 -6 0 M4 4 L20 20",
        "clock": ring + " M12 7 v5 l3.5 2",
        "archive": "M3 5 h18 v4 H3 Z M5 9 v10 h14 V9 M10 13 h4",
        "database": "M4 6 a8 3 0 1 0 16 0 a8 3 0 1 0 -16 0 M4 6 v6 a8 3 0 0 0 16 0 V6 M4 12 v6 a8 3 0 0 0 16 0 v-6",
        "stop": "M7 7 h10 v10 H7 Z",
        "terminal": "M4 5 h16 v14 H4 Z M8 10 l3 2.5 l-3 2.5 M13 15 h4",
        "hourglass": "M7 4 h10 M7 20 h10 M8 4 v3 l4 5 l4 -5 V4 M8 20 v-3 l4 -5 l4 5 v3",
        "list": "M4 6 h16 M4 12 h16 M4 18 h16",
        "key": "M3.5 15.5 a4 4 0 1 0 8 0 a4 4 0 1 0 -8 0 M10.5 12.5 L20 3 M16 7 l3 3 M13.5 9.5 l2 2",
        "panel": "M4 5 h16 v14 H4 Z M14 5 v14"
    })

    function path(name: string): string {
        return paths[name] ?? ""
    }
}
