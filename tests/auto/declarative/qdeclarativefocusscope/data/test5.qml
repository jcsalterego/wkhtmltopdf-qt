import Qt 4.7

Rectangle {
    color: "white"
    width: 800
    height: 600

    Keys.onReturnPressed: console.log("Error - Root")

    FocusScope {
        id: myScope
        focus: true

        Keys.onReturnPressed: console.log("Error - FocusScope")

        Rectangle {
            objectName: "item0"
            height: 120
            width: 420

            color: "transparent"
            border.width: 5
            border.color: myScope.wantsFocus?"blue":"black"

            Rectangle {
                x: 10; y: 10
                width: 100; height: 100; color: "green"
                border.width: 5
                border.color: item1.wantsFocus?"blue":"black"
            }

            TextEdit {
                id: item1; objectName: "item1"
                x: 20; y: 20
                width: 90; height: 90
                color: "white"
                font.pixelSize: 20
                Keys.onReturnPressed: console.log("Top Left");
                KeyNavigation.right: item2
                focus: true
                wrapMode: TextEdit.WordWrap
                text: "Box 1"
            }

            Rectangle {
                id: item2; objectName: "item2"
                x: 310; y: 10
                width: 100; height: 100; color: "green"
                border.width: 5
                border.color: wantsFocus?"blue":"black"
                KeyNavigation.left: item1
                Keys.onReturnPressed: console.log("Top Right");

                Rectangle {
                    width: 50; height: 50; anchors.centerIn: parent
                    color: parent.focus?"red":"transparent"
                }
            }
        }
        KeyNavigation.down: item3
    }

    Text { x:100; y:170; text: "Blue border indicates scoped focus\nBlack border indicates NOT scoped focus\nRed box or flashing cursor indicates active focus\nUse arrow keys to navigate\nPress Ctrl-Return to print currently focused item" }

    Rectangle {
        x: 10; y: 300
        width: 100; height: 100; color: "green"
        border.width: 5
        border.color: item3.wantsFocus?"blue":"black"
    }

    TextEdit {
        id: item3; objectName: "item3"
        x: 20; y: 310
        width: 90; height: 90
        color: "white"
        font.pixelSize: 20
        text: "Box 3"

        Keys.onReturnPressed: console.log("Bottom Left");
        KeyNavigation.up: myScope
        wrapMode: TextEdit.WordWrap
    }
}
