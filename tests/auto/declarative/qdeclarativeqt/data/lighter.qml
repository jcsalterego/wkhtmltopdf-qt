import Qt 4.7

QtObject {
    property variant test1: Qt.lighter(Qt.rgba(1, 0.8, 0.3))
    property variant test2: Qt.lighter()
    property variant test3: Qt.lighter(Qt.rgba(1, 0.8, 0.3), 10)
    property variant test4: Qt.lighter("red");
    property variant test5: Qt.lighter("perfectred"); // Non-existant color
    property variant test6: Qt.lighter(10);
}
