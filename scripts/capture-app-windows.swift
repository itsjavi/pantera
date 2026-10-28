#!/usr/bin/env swift
// Saves each on-screen window of one process to a PNG, by window ID, so a screenshot shows only
// that process even when other windows cover it. Used for screenshots of Pantera's UI, together
// with developer switches such as --pantera-show-shields-panel (AGENTS.md, Validation).
// Usage: swift scripts/capture-app-windows.swift <pid> <output prefix>
// Pass the PID of the copy you launched: matching by app name would also catch the windows of
// an installed Pantera that's running. Writes <output prefix>-1.png, -2.png, … front to back, and
// prints each window's size. The terminal running it needs the Screen Recording permission.

import CoreGraphics
import Foundation

let arguments = CommandLine.arguments
guard arguments.count == 3 else {
  FileHandle.standardError.write(
    "Usage: capture-app-windows.swift <pid> <output prefix>\n".data(using: .utf8)!)
  exit(1)
}
guard let pid = Int(arguments[1]) else {
  FileHandle.standardError.write("Not a PID: \(arguments[1])\n".data(using: .utf8)!)
  exit(1)
}
let prefix = arguments[2]

let windows =
  CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID)
  as? [[String: Any]] ?? []
var saved = 0
for window in windows where window[kCGWindowOwnerPID as String] as? Int == pid {
  guard let id = window[kCGWindowNumber as String] as? Int,
    let bounds = window[kCGWindowBounds as String] as? [String: Double],
    let width = bounds["Width"], let height = bounds["Height"],
    width >= 50, height >= 50
  else { continue }
  saved += 1
  let path = "\(prefix)-\(saved).png"
  let capture = Process()
  capture.executableURL = URL(fileURLWithPath: "/usr/sbin/screencapture")
  // -x: no sound, -o: no window shadow, -l: just this window.
  capture.arguments = ["-x", "-o", "-l\(id)", path]
  try capture.run()
  capture.waitUntilExit()
  print("\(path): \(Int(width))x\(Int(height))")
}
if saved == 0 {
  FileHandle.standardError.write("No windows of process \(pid) on screen\n".data(using: .utf8)!)
  exit(1)
}
