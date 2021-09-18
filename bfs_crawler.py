from googlesearch import search
from bs4 import BeautifulSoup
import concurrent.futures
from urllib.parse import urlparse, urljoin
import urllib.robotparser
import urllib.request
import queue
import time
start_time = time.time()

file = open("bfs_cralwer_result.txt","w")

def finish_time():
    print("--- Finished in %s seconds ---" % (time.time() - start_time))

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

def create_robot_parser(url):
    rp = urllib.robotparser.RobotFileParser()
    rp.set_url(url+"/robots.txt")
    rp.read()
    return rp

def crawl(url, index):
    try:
        page=urllib.request.urlopen(url)
    except urllib.error.HTTPError as error:
        print(index, url+" "+str(error.code))
        return None
    except urllib.error.URLError:
        print(index, url+" "+"URLError")
        return None
    content=page.read()
    size=len(content)/1024
    print(index, url+" Size: "+"{:.2f}".format(size)+"kb")
    return content,url

def get_links(page_content,url):
    if page_content is None:
        return []
    soup = BeautifulSoup(page_content, "html.parser")
    #if there is a base tag, fetch it
    base = soup.find_all('base')
    #use a set to filter out duplicates
    linkset=set()
    if base:
        print(base)
        url=base[0].get('href')
    base_parse_result=urlparse(url)
    current_site=base_parse_result.scheme+"://"+base_parse_result.netloc
    robot_parser=create_robot_parser(current_site)
    for link in soup.find_all('a',href=True):
        #filter the links
        #non-url
        #modify relative url to common url
        href=link.get('href')
        parse_result=urlparse(href)
        if not href:
            continue
        if href[0]!="/" and href[0]!="#":
            if href[:4] != "http":
                continue
        if parse_result.scheme=="":
            link_to_add=urljoin(url,href)
        else:
            link_to_add=href
        # print(link_to_add)
        if robot_parser.can_fetch("*",link_to_add):
            linkset.add(link_to_add)
            # print(href)
    return list(linkset)

# result=crawl("https://www.dnb.com/business-directory/company-profiles.the_brooklyn_union_gas_company.89887e0b5ad7b8157fc9e5f233efd2a8.html",1)
# if result!=None:
#     content,url=result
#     print(content,url)
# else:
#     content=None
#     url=None
# print(len(get_links(content,url)))

def bfs_crawler(query):
    results_list=google_search(query)
    seen=set()
    scheduled_crawl=[]
    #store number of time a site is crawled
    crawled_site={}
    q=queue.Queue()
    counter=0
    batch=queue.Queue()
    result_count=0
    qsize=0
    for link in results_list:
        parse_result=urlparse(link)
        parsed_link=urljoin(parse_result.scheme+"://"+parse_result.netloc,parse_result.path)
        netloc=parse_result.netloc
        if parsed_link not in seen and netloc not in crawled_site or crawled_site[netloc]<10:
            q.put(link)
            qsize+=1
            seen.add(link)
            if netloc not in crawled_site:
                crawled_site[netloc]=1
            else:
                crawled_site[netloc]+=1
    with concurrent.futures.ThreadPoolExecutor() as executor:
        while result_count<5000 and not q.empty():
            for _ in range(max(10,result_count)):
                cur_url=q.get()
                qsize-=1
                scheduled_crawl.append((counter,cur_url))
                counter+=1

            batch_results=[executor.submit(crawl,url,index) for index,url in scheduled_crawl]
            
            for job in concurrent.futures.as_completed(batch_results):
                crawl_result=job.result()
                if crawl_result is not None:
                    batch.put(crawl_result)
                result_count+=1
            

            while not batch.empty() and qsize<result_count:
                crawled_content=batch.get()
                if not crawled_content:
                    continue
                content,url=crawled_content[0],crawled_content[1]
                for link in get_links(content,url):
                    if link:
                        parse_result=urlparse(link)
                        netloc=parse_result.netloc
                        if link not in seen:
                            #up to 10 page from a netloc can be crawled
                            if netloc not in crawled_site or crawled_site[netloc]<10:
                                q.put(link)
                                # print(link)
                                qsize+=1
                                seen.add(link)
                                if netloc not in crawled_site:
                                    crawled_site[netloc]=1
                                else:
                                    crawled_site[netloc]+=1
                # print("qsize:", qsize)
                print(result_count,qsize)
                finish_time()
                if qsize>=result_count:
                    break
            
bfs_crawler("brooklyn union")
finish_time()