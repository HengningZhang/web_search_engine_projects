List of files:
readme.txt -- you are reading it
explain.txt -- explanation of how the code works
crawler.py -- Source code
bfs_crawler_result_1.txt -- log of running bfs crawler on "brooklyn union"
bfs_crawler_result_2.txt -- log of running bfs crawler on "paris texas"
prioritized_crawler_result_1.txt -- log of running prioritized crawler on "brooklyn union"
prioritized_crawler_result_2.txt -- log of running prioritized crawler on "paris texas"
prioritized_crawler_result_1_no_wiki.txt -- log of running prioritized crawler on "brooklyn union" while excluding all links that contains "wiki"
requirements.txt -- to make installing requirements easier

make sure you have the following packages installed: (easier install using pip install -r requirements.txt)
google
bs4
urllib
validators


How to run the code:
At the end of crawler.py, you can set the log file output path
Then run the bfs_crawler() or prioritized_crawler() in the try-except blocks.
They are there because I want the total size and total time in the log even if the crawler encounter an exception.
Input for bfs_crawler() or prioritized_crawler has to be a single string but there is no size limit.

You will see some output in the terminal indicating the current progress.

