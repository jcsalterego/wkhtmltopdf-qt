import Qt 4.7

QtObject {
    property variant test1: Qt.darker(Qt.rgba(1, 0.8, 0.3))
    property variant test2: Qt.darker()
    property variant test3: Qt.darker(Qt.rgba(1, 0.8, 0.3), 10)
    property variant test4: Qt.darker("red");
    property variant test5: Qt.darker("perfectred"); // Non-existant color
    property variant test6: Qt.darker(10);
}

