# LOGREPAIR README


## Basic Overview

1. open file a, file b from param
2. get file and timestamp metadata (used to reduce memory allocation, rather than pulling full file in, and reduce delay on disk - instead of reading whole file all at once, we can read a "block" from first to last instance of a particular timestamp to get all of its instances, sometimes this could be slow (if for example for some reason the same timestamp is very early and very late in the file) -- this could also be sped up by recording all of the instances of a timestamp, probably, will look in to that later.
3. using 'metadata', grab blocks of lines (from first to last instance) and record only the ones starting with the timestamp

### Function: 

* sort by timestamp, retaining original order if timestamp matches (ie. if it were an a spreadsheet, you would add a new column that holds the original line number, then sort by timestamp column followed by the line number column)
* then compare line by line
* if line doesn't match, forward search until you find a match, or until the timestamp differs.
    * ... If match not found (ran into next timestamp), use the record.
    * ... if match found, note that line as being written already and use that line.

### Goals:

* optimize memory usage
* optimize disk pulls (max 100 results/page currently -- could be higher or lower.)
* support multi-line log entries
