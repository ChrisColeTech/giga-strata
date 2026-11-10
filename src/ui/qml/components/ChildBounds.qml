import QtQuick

// The family kit's lib/child-bounds.ts, ported: where a child window may be and how large.
// Boxes are {x, y, width, height} in window pixels; edges are Qt.Edge flags (the kit's n/s/e/w).
// `spec` is the kit's ChildSpec: {width, body, minWidth, minBody}. The room is the whole client
// area: this app keeps its native title bar, so there is no titlebar strip to leave out.
QtObject {
    id: root

    // What the window's padding and border add around its content on each axis.
    readonly property int chrome: 2 * (Theme.childInset + Theme.childBorder)

    // The outer size of a window with `width` of content and `body` under its header.
    function sized(width: real, body: real): var {
        return { width: width + root.chrome, height: Theme.childTitleHeight + Theme.childHeaderGap + body + root.chrome }
    }

    function roomOf(viewport: var): var {
        return { x: 0, y: 0, width: viewport.width, height: Math.max(0, viewport.height) }
    }

    // The main window minus its status bar, derived from the viewport so a resize keeps it fitted.
    function maximizedBox(viewport: var): var {
        const room = root.roomOf(viewport)
        return { x: room.x, y: room.y, width: room.width, height: Math.max(0, room.height - Theme.statusBarHeight) }
    }

    // `max` is the room; `min` is the surface's least, capped to `max`; `open` is the content size, at most
    // `childOpenShare` of the room and never below `min`.
    function limitsOf(spec: var, room: var): var {
        const max = { width: room.width, height: room.height }
        const least = root.sized(spec.minWidth, spec.minBody)
        const min = { width: Math.min(least.width, max.width), height: Math.min(least.height, max.height) }
        const want = root.sized(spec.width, spec.body)
        const open = {
            width: Math.max(min.width, Math.min(want.width, Math.floor(room.width * Theme.childOpenShare))),
            height: Math.max(min.height, Math.min(want.height, Math.floor(room.height * Theme.childOpenShare)))
        }
        return { open: open, min: min, max: max }
    }

    function clampSize(size: var, limits: var): var {
        return {
            width: Math.max(limits.min.width, Math.min(size.width, limits.max.width)),
            height: Math.max(limits.min.height, Math.min(size.height, limits.max.height))
        }
    }

    // The top-left for a window of `size`, moved the least that puts the whole window inside `room`.
    function inside(x: real, y: real, size: var, room: var): var {
        return {
            x: Math.max(room.x, Math.min(x, room.x + room.width - size.width)),
            y: Math.max(room.y, Math.min(y, room.y + room.height - size.height))
        }
    }

    function placeAt(size: var, room: var): var {
        return root.inside(room.x + (room.width - size.width) / 2, room.y + (room.height - size.height) / 2, size, room)
    }

    // Where and how large it last was (clamped and put back inside the room), else centred at the size its
    // content wants.
    function openBox(spec: var, room: var, kept: var): var {
        const limits = root.limitsOf(spec, room)
        const size = root.clampSize(kept === null ? limits.open : { width: kept.width, height: kept.height }, limits)
        const at = kept === null ? root.placeAt(size, room) : root.inside(kept.x, kept.y, size, room)
        return { x: at.x, y: at.y, width: size.width, height: size.height }
    }

    // `box` after dragging `edges` by (dx, dy), held between the limits and inside the room. A west or north drag
    // moves the opposite edge's anchor, not the window, so the far side stays where it was.
    function resized(box: var, edges: int, dx: real, dy: real, limits: var, room: var): var {
        let x = box.x
        let y = box.y
        let width = box.width
        let height = box.height
        if (edges & Qt.RightEdge) {
            width = Math.max(limits.min.width, Math.min(box.width + dx, limits.max.width, room.x + room.width - box.x))
        }
        if (edges & Qt.BottomEdge) {
            height = Math.max(limits.min.height, Math.min(box.height + dy, limits.max.height, room.y + room.height - box.y))
        }
        if (edges & Qt.LeftEdge) {
            const right = box.x + box.width
            width = Math.max(limits.min.width, Math.min(box.width - dx, limits.max.width, right - room.x))
            x = right - width
        }
        if (edges & Qt.TopEdge) {
            const bottom = box.y + box.height
            height = Math.max(limits.min.height, Math.min(box.height - dy, limits.max.height, bottom - room.y))
            y = bottom - height
        }
        return { x: x, y: y, width: width, height: height }
    }
}
