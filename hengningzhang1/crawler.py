from googlesearch import search
from bs4 import BeautifulSoup
import concurrent.futures
from urllib.parse import urlparse, urljoin
import urllib.robotparser
import urllib.request
import queue
import time
import socket
import validators
import heapq
from datetime import datetime

start_time = time.time()
date = datetime.today()

total_size=[0]

#found on https://stackoverflow.com/questions/12140460/how-to-skip-some-file-type-while-crawling-with-scrapy
IGNORED_EXTENSIONS = [
    # images
    'mng', 'pct', 'bmp', 'gif', 'jpg', 'jpeg', 'png', 'pst', 'psp', 'tif',
    'tiff', 'ai', 'drw', 'dxf', 'eps', 'ps', 'svg',

    # audio
    'mp3', 'wma', 'ogg', 'wav', 'ra', 'aac', 'mid', 'au', 'aiff',

    # video
    '3gp', 'asf', 'asx', 'avi', 'mov', 'mp4', 'mpg', 'qt', 'rm', 'swf', 'wmv',
    'm4a', 'webm', #this one I found on wikipedia sites myself ,

    # other
    'css', 'pdf', 'doc', 'exe', 'bin', 'rss', 'zip', 'rar',
]

def finish_time():
    print("--- Finished in %s seconds ---" % (time.time() - start_time))

#self explanatory, logging completion time for each log entry
def print_current_datetime():
    try:
        time=datetime.now()
        file.write(" "+date.strftime("%b-%d-%Y")+" "+time.strftime("%H:%M:%S")+"\n")
    except:
        return

#check if the url's path is an unwanted file type
def is_unwanted_file(path):
    res=path.split(".")
    if len(res)==1:
        return False
    if res[-1].lower() in IGNORED_EXTENSIONS:
        return True
    return False

#copied from online, reference in explain.txt
def google_search(query):
    print("--- running google search ---")
    results_list = []
    for result in search(query,        # The query you want to run
                    lang = 'en',  # The language
                    num = 10,     # Number of results per page
                    start = 0,    # First result to retrieve
                    stop = 10,  # Last result to retrieve
                
                ):
        results_list.append(result)
    return results_list

#create the 
def create_robot_parser(url):
    rp = urllib.robotparser.RobotFileParser()
    rp.set_url(url+"/robots.txt")
    rp.read()
    return rp

def crawl(url, index, priority=0):
    #open the page using urllib
    #catches common httperror and urlerrors
    #don't want to let the crawler stop at weird exceptions
    try:
        page=urllib.request.urlopen(url,timeout=1)
    except socket.timeout:
        file.write(str(index)+" "+ url+" connection time out")
        print_current_datetime()
        return None
    except urllib.error.HTTPError as error:
        file.write(str(index)+" "+ url+" ErrorCode:"+str(error.code))
        print_current_datetime()
        return None
    except urllib.error.URLError:
        file.write(str(index)+" "+ url+" "+"URLError")
        print_current_datetime()
        return None
    except:
        file.write(str(index)+" "+ url+" "+"Other weird Error")
        print_current_datetime()
        return None
    try:
        content=page.read()
    except:
        return None
    #calculate the size and log in a file
    size=len(content)/1024
    file.write(str(index)+" "+ url+" Size: "+"{:.2f}".format(size)+"kb"+" Return code:200")
    if priority!=0:
        file.write(" Priority score:"+"{:.2f}".format(priority)+" ")
    print_current_datetime()
    total_size[0]+=size
    return content,url

def get_links(page_content,url):
    if page_content is None:
        return []
    try:
        soup = BeautifulSoup(page_content, "html.parser")
    except:
        return []
    #if there is a base tag, fetch it
    base = soup.find_all('base')
    #use a set to filter out duplicates
    linkset=set()
    if base:
        try:
            url=base[0].get('href')
            if not validators.url(url):
                return []
        except:
            return []
        
    base_parse_result=urlparse(url)
    current_site=base_parse_result.scheme+"://"+base_parse_result.netloc
    try:
        robot_parser=create_robot_parser(current_site)
    except:
        return []
    for link in soup.find_all('a',href=True):
        #filter the links
        #non-url
        #modify relative url to common url
        href=link.get('href')
        parse_result=urlparse(href)
        if not href:
            continue
        if is_unwanted_file(parse_result.path):
            continue
        # used out of curious
        # if "wiki" in href or "wiki" in url:
        #     continue
        if href[0]!="/" and href[0]!="#":
            if href[:4] != "http":
                continue
        if parse_result.scheme=="":
            link_to_add=urljoin(url,parse_result.path)
        else:
            link_to_add=href
        #fixes
        #using chrome user agent now 
        if robot_parser.can_fetch("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36",link_to_add):
            linkset.add(link_to_add)
    return list(linkset)

def bfs_crawler(query):
    results_list=google_search(query)
    seen=set() #used to check if a specific link is already crawled
    scheduled_crawl=[]
    #store number of time a site is crawled
    crawled_site={} # hashmap to record the amount of pages crawled from each site
    q=queue.Queue() # queue of links that have not been crawled yet
    counter=0 # not actually necessary, just for my curiosity about multithreading
    crawled_pages=queue.Queue() #crawled but unparsed pages
    result_count=0
    qsize=0 #just for console output purposes
    #initialize the q with google search results
    for link in results_list:
        parse_result=urlparse(link)
        netloc=parse_result.netloc
        if link not in seen and (netloc not in crawled_site or crawled_site[netloc]<10):
            q.put(link)
            qsize+=1
            seen.add(link)
            if netloc not in crawled_site:
                crawled_site[netloc]=1
            else:
                crawled_site[netloc]+=1
    
    with concurrent.futures.ThreadPoolExecutor() as executor:
        while result_count<12000 and not q.empty():
            scheduled_crawl.clear()
            #multithreading the downloading process
            for _ in range(10):
                cur_url=q.get()
                qsize-=1
                scheduled_crawl.append((counter,cur_url))
                counter+=1
            crawled_pages_results=[executor.submit(crawl,url,index) for index,url in scheduled_crawl]
            for job in concurrent.futures.as_completed(crawled_pages_results):
                crawl_result=job.result()
                if crawl_result is not None:
                    crawled_pages.put(crawl_result)
                result_count+=1
            #parse some unparsed page content so there are more to crawl
            while not crawled_pages.empty() and qsize<10:
                crawled_content=crawled_pages.get()
                if not crawled_content:
                    continue
                content,url=crawled_content[0],crawled_content[1]
                for link in get_links(content,url):
                    if link:
                        try:
                            parse_result=urlparse(link)
                        except:
                            continue
                        netloc=parse_result.netloc
                        if link not in seen:
                            #up to 10 page from a netloc can be crawled
                            if netloc not in crawled_site or crawled_site[netloc]<10:
                                q.put(link)
                                qsize+=1
                                seen.add(link)
                                if netloc not in crawled_site:
                                    crawled_site[netloc]=1
                                else:
                                    crawled_site[netloc]+=1
                #printing out progress in console, so the user is not bored or panicking
                print("Crawled pages:",result_count,"unparsed pages:",crawled_pages.qsize(),"Qsize:",qsize)
                finish_time()
        file.write("File count:"+str(result_count)+", Total size:"+"{:.2f}".format(total_size[0]/1024)+"Mb")

novelty_res={}

#Object-ize the urls so that they can be compared, in order to use the heapq library
class ScoredUrl:
    def __init__(self, url):
        self.url=url
        self.netloc=urlparse(url).netloc
        self.importance=0
        if self.netloc not in novelty_res:
            novelty_res[self.netloc]=1
        else:
            novelty_res[self.netloc]+=1

    def __eq__(self,other):
        return self.get_priority_score()==other.get_priority_score()

    def __lt__(self,other):
        return -self.get_priority_score()<-other.get_priority_score()

    def __gt__(self,other):
        return other<self

    def add_importance(self):
        self.importance+=1

    def get_priority_score(self):
        return 5/novelty_res[self.netloc]+self.importance

def prioritized_crawler(query):
    google_results_list=google_search(query)
    seen=set()
    scheduled_crawl=[]
    crawled_site={} #store number of time a site is crawled
    counter=0 
    crawled_pages=queue.Queue() #crawled but unparsed pages
    result_count=0 
    scored_urls={} #hashmap to those scored urls that have not been crawled
    priority_queue=[] #use heapq library to implement the priority queue
    #initializing queue with google search results
    for link in google_results_list:
        netloc=urlparse(link).netloc
        if link not in seen and (netloc not in crawled_site or crawled_site[netloc]<10):
            cur_scored_url=ScoredUrl(link)
            priority_queue.append(cur_scored_url)
            scored_urls[link]=cur_scored_url
            seen.add(link)
            if netloc not in crawled_site:
                crawled_site[netloc]=1
            else:
                crawled_site[netloc]+=1
    heapq.heapify(priority_queue)

    with concurrent.futures.ThreadPoolExecutor() as executor:
        while result_count<12000 and not len(priority_queue)==0:
            #multithreaded downloading process
            scheduled_crawl.clear()
            for _ in range(10):
                cur_scored_url=heapq.heappop(priority_queue)
                cur_url=cur_scored_url.url
                scored_urls.pop(cur_url)
                counter+=1
                scheduled_crawl.append((counter,cur_url,cur_scored_url.get_priority_score()))

            crawled_pages_results=[executor.submit(crawl,url,index,priority) for index,url,priority in scheduled_crawl]
            
            for job in concurrent.futures.as_completed(crawled_pages_results):
                crawl_result=job.result()
                if crawl_result is not None:
                    crawled_pages.put(crawl_result)
                result_count+=1
            
            #fill the priority queue to 1000 or exhaust crawled pages
            while not crawled_pages.empty() and len(priority_queue)<1000:
                crawled_content=crawled_pages.get()
                if not crawled_content:
                    continue
                content,url=crawled_content[0],crawled_content[1]
                for link in get_links(content,url):
                    if link:
                        try:
                            parse_result=urlparse(link)
                        except:
                            continue
                        netloc=parse_result.netloc
                        if link in scored_urls:
                            #if a link is already added to the priority queue, increment its importance
                            scored_urls[link].add_importance()
                            continue
                        if link not in seen:
                            #up to 10 page from a netloc can be crawled
                            if netloc not in crawled_site or crawled_site[netloc]<10:
                                cur_scored_url=ScoredUrl(link)
                                priority_queue.append(cur_scored_url)
                                #map it in a hashmap so we can do the add_importance()
                                scored_urls[link]=cur_scored_url
                                seen.add(link)
                                if netloc not in crawled_site:
                                    crawled_site[netloc]=1
                                else:
                                    crawled_site[netloc]+=1
            
                
                print("Crawled pages:",result_count,"unparsed pages:",crawled_pages.qsize(),"Qsize:",len(priority_queue))
                finish_time()
            #only heapify after all the new links are pushed into the priority queue
            heapq.heapify(priority_queue)
        file.write("File count:"+str(result_count)+", Total size:"+"{:.2f}".format(total_size[0]/1024)+"Mb, Total time used: "+"{:.2f}".format(time.time() - start_time)+" seconds")


# fun part
# put all the mess above into use

# set the file path you want the log to be
FILE_ADDRESS="bfs_crawler_result_1_1.txt"
QUERY="flatbush"
file = open(FILE_ADDRESS,"w",encoding='utf-8')

# choose which one you want to run
# left them in the try-except block because I want the total size and total time even if they fail
# remember to comment out the one that is not meant to be executed this time


# page,index= crawl("https://bronx.news12.com/police-man-killed-11-year-old-among-3-others-injured-in-claremont-park-shooting",1)
# for link in get_links(page,"https://bronx.news12.com/police-man-killed-11-year-old-among-3-others-injured-in-claremont-park-shooting"):
#     print(link)
# a=urlparse("https://bronx.news12.com/category/bronx-state-of-our-schools")
# print(a)
try:
    # bfs_crawler(QUERY)
    prioritized_crawler(QUERY)
except :
    file.write("Total size:"+"{:.2f}".format(total_size[0]/1024)+"Mb ")
    file.write("Total time used: "+"{:.2f}".format(time.time() - start_time)+" seconds")

finish_time()
