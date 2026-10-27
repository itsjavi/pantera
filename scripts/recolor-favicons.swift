#!/usr/bin/env swift
// Recolors the blue favicons of Chromium's own pages (Settings, Downloads, History...) in
// Pantera's amber and writes them to branding/overlay/, keeping their shape and alpha.
// Usage: swift scripts/recolor-favicons.swift build/src branding/overlay
// Run it after an upstream sync, against an unmodified Chromium checkout, and commit the PNGs.
// The amber is tone 55 of DECISION-8's palette: at least 3:1 against both light and dark tabs.
import AppKit

let arguments = CommandLine.arguments
guard arguments.count == 3 else {
    print("Usage: swift scripts/recolor-favicons.swift <chromium-src> <overlay>")
    exit(1)
}
let source = URL(fileURLWithPath: arguments[1]), overlay = URL(fileURLWithPath: arguments[2])
let amber: (r: Double, g: Double, b: Double) = (0xB3 / 255.0, 0x78 / 255.0, 0x00 / 255.0)

// Chromium's New Tab and product favicons are its logo, not blue glyphs; TASK-19 replaces them.
let favicons = ["settings", "downloads", "bookmarks", "management", "plugins"].map {
    "chrome/app/theme/%@/common/favicon_\($0).png"
} + [
    "chrome/app/theme/%@/chromium/favicon_password_manager.png",
    "components/resources/%@/favicon_history.png",
    "components/resources/%@/flags_ui/favicon_flags.png",
    "extensions/browser/resources/%@/favicon_extensions.png",
]

func recolor(_ input: URL, _ output: URL) throws {
    guard let image = NSImage(contentsOf: input)?.cgImage(forProposedRect: nil, context: nil, hints: nil)
    else { throw CocoaError(.fileReadCorruptFile) }
    let width = image.width, height = image.height
    let context = CGContext(data: nil, width: width, height: height, bitsPerComponent: 8, bytesPerRow: width * 4,
                            space: CGColorSpace(name: CGColorSpace.sRGB)!,
                            bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
    context.draw(image, in: CGRect(x: 0, y: 0, width: width, height: height))
    let pixels = context.data!.bindMemory(to: UInt8.self, capacity: width * height * 4)
    for i in stride(from: 0, to: width * height * 4, by: 4) {
        let alpha = Double(pixels[i + 3]) / 255
        guard alpha > 0 else { continue }
        // Unpremultiply to tell blue pixels from gray or white ones, which stay as they are.
        let r = Double(pixels[i]) / 255 / alpha, g = Double(pixels[i + 1]) / 255 / alpha
        let b = Double(pixels[i + 2]) / 255 / alpha
        guard b - max(r, g) > 0.12 else { continue }
        pixels[i] = UInt8((amber.r * alpha * 255).rounded())
        pixels[i + 1] = UInt8((amber.g * alpha * 255).rounded())
        pixels[i + 2] = UInt8((amber.b * alpha * 255).rounded())
    }
    try FileManager.default.createDirectory(at: output.deletingLastPathComponent(), withIntermediateDirectories: true)
    let rep = NSBitmapImageRep(cgImage: context.makeImage()!)
    try rep.representation(using: .png, properties: [:])!.write(to: output)
}

var count = 0
for template in favicons {
    for scale in ["default_100_percent", "default_200_percent"] {
        let path = String(format: template, scale)
        try recolor(source.appendingPathComponent(path), overlay.appendingPathComponent(path))
        count += 1
    }
}
print("Recolored \(count) favicons into \(overlay.path)")
