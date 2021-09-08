OSP - Programming Assingment 1
S3851781
Jeffrey Zezhong Tan

Submission Details:

(1) Reader_Writer.cpp
/** 
 * ##   README  ##
 * 
 * Problem: Starvation Problem
 * Writers may only write when the number of readers is zero.
 * 
 * After a reader thread completes a loop, it is instantly ready to read again in the following loop.
 * Suppose we have all 5 reader threads reading simultaneously, everytime the read loop is complete, 
 * the readCount in decremented to 4, only to be incremented back to 5 when the loop is run again.
 * This would make for a very uninteresting output, where the majority would be the read threads reading the 
 * resource with the write threads being unable to execute.
 * 
 * My Solution:
 * I have added sleep(1) to every read and write thread at the end of their loops.
 * This allows us to simulate a more interesting output, where there are a varying number of readers and writers
 * available at any point in time. If the reader is not automatically put into ready mode, the writers have a chance
 * to write to the resource.
 * 
*/



(2) Sleeping_Barber.cpp




Make Commands:

