import Qt 4.7

Flickable {
    width: 100; height: 100
    contentWidth: column.width; contentHeight: column.height
    pressDelay: 200; boundsBehavior: Flickable.StopAtBounds; interactive: false
    maximumFlickVelocity: 2000

    Column {
        id: column
        Repeater {
            model: 4
            Rectangle { width: 200; height: 300; color: "blue" }
        }
    }
}
