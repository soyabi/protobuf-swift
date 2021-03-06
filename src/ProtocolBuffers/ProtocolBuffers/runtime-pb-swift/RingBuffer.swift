// Protocol Buffers for Swift
//
// Copyright 2014 Alexey Khohklov(AlexeyXo).
// Copyright 2008 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License")
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import Foundation
internal class RingBuffer
{
    internal var buffer:[Byte]
    var position:Int32 = 0
    var tail:Int32 = 0
    
    init(data:[Byte])
    {
        buffer = data
    }
    func freeSpace() ->UInt32
    {
        var res:UInt32 = 0
        
        if position < tail
        {
            res = UInt32(tail - position)
        }
        else
        {
            let dataLength = buffer.count
            res = UInt32((Int32(dataLength) - position) + tail)
        }
        
        if tail != 0
        {
            res -=  1
        }
        return res
    }
    
    func appendByte(byte aByte:Byte) -> Bool
    {
        if freeSpace() < 1
        {
            return false
        }
        buffer[Int(position++)] = aByte
        return true
    }
    
    func appendData(var input:[Byte], offset:Int32, length:Int32) -> Int32
    {
        var totalWritten:Int32 = 0
        var aLength = length
        var aOffset = offset
        if (position >= tail)
        {
            totalWritten = min(Int32(buffer.count) - Int32(position), Int32(aLength))
            memcpy(&buffer + Int(position), &input + Int(aOffset), UInt(totalWritten))
            position += totalWritten
            if totalWritten == aLength
            {
                return aLength
            }
            aLength -= Int32(totalWritten)
            aOffset += Int32(totalWritten)
            
        }
        
        let freeSpaces:UInt32 = freeSpace()
        
        if freeSpaces == 0
        {
            return totalWritten
        }
        
        if (position == Int32(buffer.count)) {
            position = 0
        }
        
        let written:Int32 = min(Int32(freeSpaces), aLength)
        memcpy(&buffer + Int(position), &input + Int(aOffset), UInt(written))
        position += written
        totalWritten += written
        
        return totalWritten
    }
    
    func flushToOutputStream(stream:NSOutputStream) ->Int32
    {
        var totalWritten:Int32 = 0
        
        var data = buffer
        if tail > position
        {
            var written:Int = stream.write(&data + Int(tail), maxLength:Int(buffer.count - Int(tail)))
            if written <= 0
            {
                return totalWritten
            }
            totalWritten+=Int32(written)
            tail += Int32(written)
            if (tail == Int32(buffer.count)) {
                tail = 0
            }
        }
        
        if (tail < position) {
            
            var written:Int = stream.write(&data + Int(tail), maxLength:Int(position - tail))
            if (written <= 0)
            {
                return totalWritten
            }
            totalWritten += Int32(written)
            tail += Int32(written)
        }
        
        if (tail == position) {
            tail = 0
            position = 0
        }
        
        if (position == Int32(buffer.count) && tail > 0) {
            position = 0
        }
        
        if (tail == Int32(buffer.count)) {
            tail = 0
        }
        
        return totalWritten
    }
    
    
}
