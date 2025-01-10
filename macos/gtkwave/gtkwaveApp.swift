//
//  macosApp.swift
//  macos
//
//  Created by Emin017 on 2025/1/9.
//
//  Ref: https://github.com/kattrali/rust-mac-app-examples/blob/master/3-packaging-a-mac-app/app/main.swift

import Cocoa

@main
class macOSApp {
    static func main() {
        let task = Process()
        let bundle = Bundle.main

        if let resourceURL = bundle.resourceURL?.appendingPathComponent("bin/gtkwave") {
            task.executableURL = resourceURL
        } else {
            fatalError("Binary 'gtkwave' not found in Resources/app folder")
        }

        let stdOut = Pipe()
        let stdErr = Pipe()

        stdOut.fileHandleForReading.readabilityHandler = { fileHandle in
            log(fileHandle.availableData)
        }

        stdErr.fileHandleForReading.readabilityHandler = { fileHandle in
            log(fileHandle.availableData)
        }

        task.standardOutput = stdOut
        task.standardError = stdErr

        task.terminationHandler = { task in
            (task.standardOutput as AnyObject?)?.fileHandleForReading.readabilityHandler = nil
            (task.standardError as AnyObject?)?.fileHandleForReading.readabilityHandler = nil
        }

        do {
            try task.run() // Start task
            task.waitUntilExit() // Keep syncing, until task end
        } catch {
            NSLog("Failed to start task: \(error)")
        }
    }
    // Log function
    static func log(_ data: Data) {
        if let message = String(data: data, encoding: .utf8) {
            NSLog("%@", message)
        }
    }
}
