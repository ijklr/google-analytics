//Author: Shaun Yi Cheng cheng754@gmail.com
//Date: 8/9/2016
//This is a html report generation program.
//It reads 5 files: 
//start_dates.txt for starting dates,
//view_ids.txt for view ids
//whitelist.txt for white-listed url matching
//blacklist.txt for black-listed url matching


#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <regex>
#include <map>
#include <fstream>
#include <numeric>
#include <sstream>
#include <set>
using namespace std;

struct Rec {
    string name;
    int date=0;
    int count=0;
    int newUsers;
    int sessions;
    double bounceRate;
    int pageviews;
    double pageviewsPerSession;
    double avgSessionDuration;
    string url;
    string medium;
    string socialNetWork;
    string str() const {
	ostringstream oss;
	oss 
	<<"  name = "<<name
	<<"  count = "<<count
	<<"  new Users = "<<newUsers
	<<"  sessions = "<<sessions
	<<"  bounceRate = "<<bounceRate
	<<"  medium= "<< medium
	<<"  pageviews = "<<pageviews
	<<"  pageviewsPerSession!! = " << pageviewsPerSession 
	<<"  avgSessionDuration = " << avgSessionDuration 
	<<"  url = " << url
	<<"  socialNetWork = " << socialNetWork;
	return oss.str();
    }
};


struct Count {
    int count=0;
    int all_count = 0;
};

//filtered out spam already
struct Kpi {
    int total_users=0;
    double pages_per_session=0;
    double bounce_rate=0;
    double percent_new=0;
    double avg_time_on_site=0;
    int referral_users=0;
    int direct_users=0;
    int social_users=0;
    int organic_search_users=0;
    /*
    int cpa_users=0;
    int cpc_users=0;
    int cpm_users=0;
    int cpp_users=0;
    int cpv_users=0;
    int ppc_users=0;
    */
    int other_users=0;
    int paid_users=0;
};


using KpiMap = map<string, map<int, Kpi>>;
using NameCount = unordered_map<string, Count>;
using DateCount = map<int, Count>;
using DateNameHash = map<int, NameCount>;
using NameDateHash = map<string, DateCount>;

using NameDateUrlCount= map<string, map<int, map<string, int>>>;

void populate_name_date_url_count(NameDateUrlCount &mmm, const vector<Rec> &recs) 
{
    for(auto &r:recs) {
	mmm[r.name][r.date][r.url] += r.count;
	mmm["all"][r.date][r.url] += r.count;
    }
}

void populate_name_date_url_avg_time(NameDateUrlCount &mmm, const vector<Rec> &recs) 
{
    for(auto &r:recs) {
	mmm[r.name][r.date][r.url] += r.avgSessionDuration;
    }
}

void url_table_header(ostream &os)
{
    ostringstream html_junk;
    html_junk << "<head> <style> table { font-family: arial, sans-serif; border-collapse: collapse; width: 100%; }td, th {    border: 1px solid #dddddd;    text-align: left; padding: 8px; }\n";
    html_junk << " tr:nth-child(even) { background-color: #dddddd; } </style> </head> <body>\n"<<endl;
    string hello{"hello"};
    html_junk << "<table> <tr> <th>Date</th> ";
    for(int i=1; i<=50; ++i) {
	html_junk <<"<th> #"<<i<<" source </th> ";
    }
    html_junk <<"</tr> \n";
    os<<html_junk.str();


}
void html_from_name_date_url_count(ostream &os, const NameDateUrlCount &mmm, const string &name)
{
    url_table_header(os);
    auto found = mmm.find(name);
    if(found != end(mmm)) {
	for(auto date:found->second) {
	    os<<"<tr>\n"
		<<"<td>"<< date.first <<"</td>\n";
	    multimap<int, string, greater<int> > sorter;
	    for(auto &p:date.second) {
		sorter.insert({p.second, p.first});
	    }
	    int i = 0;
	    for(auto it=begin(sorter); it!=end(sorter); ++it) {
		os<<"<td>"<< it->first<<" : "<<it->second <<"</td>\n";
		if(++i >=50) break;
	    }
	    os<<"</tr>\n";
	}
    }
    os<<"</table>";
}


struct pic_attr {
    string name;
    string title;
};

struct stats {
    double mean;
    double stdev;
    double sum;
};

//function prototypes:
template <class T>
void create_csv(const vector<vector<T>> &data, string filename);
void create_plg_file(const string &filename_without_ext, const string &title, const string &legend1, const string &legend2, vector<pic_attr> &html_stuff, string big_title);


double get_projection_ratio()
{
    time_t t = time(NULL);
    tm* timePtr = localtime(&t);
    double ratio = 31/(double)timePtr->tm_mday;
    return ratio;
}

//FUN PART
void calc_stats(stats &st, const vector<int> &v)
{
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    double mean = sum / v.size();
    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
    double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / v.size());
    st.mean = mean;
    st.stdev = stdev;
    st.sum = sum;
}

void do_some_stats(const NameDateHash &name_date_hash, const map<string, int> &start_dates_map, vector<pic_attr> &html_stuff)
{
    vector<vector<Count>> vv{120}; 
    for(auto &dude:name_date_hash) {
	cout<<"processing.. "<< dude.first<<endl;
	auto date_it = start_dates_map.find(dude.first);
	if(date_it != end(start_dates_map)) {
	    int start_date = date_it->second;
	    auto it = dude.second.find(start_date);
	    if(it == end(dude.second)) {
		cout<<"start date for "<< start_date<<" not found! Fix the data in start_dates.txt!\n";
		continue;
	    }
	    int i=0;
	    //double prev_count = it->second.count;
	    //double prev_all_count = it->second.all_count;
	    while(it != end(dude.second)) {
		Count c;
		c.count = it->second.count;
		c.all_count = it->second.all_count;
		//prev_count = it->second.count;
		//prev_all_count = it->second.all_count;
		vv[i++].push_back(c);
		++it;
	    }
	}
    }

    vector<vector<double>> data;
    vector<vector<double>> data1;
    vector<vector<double>> data2;
    size_t sz = 0;
    if(!vv.empty()) sz = vv[0].size();
    for(auto &v:vv){
	if(v.empty()) break;
	stats st1,st2;
	vector<int> count_v;
	vector<int> all_count_v;

	size_t data_points = 0;
	for(auto &c:v){
	    count_v.push_back(c.count);
	    all_count_v.push_back(c.all_count);
	    ++data_points;
	}
	if(data_points<9) break; //not accurate if too few data points.

	calc_stats(st1, count_v);
	calc_stats(st2, all_count_v);
	vector<double> d, d1, d2;
	d.push_back(st1.sum *sz/data_points);
	d.push_back(st2.sum *sz/data_points);
	data.push_back(d);

	d1.push_back(st1.mean + st1.stdev);
	d1.push_back(st1.mean - st1.stdev);
	data1.push_back(d1);

	d2.push_back(st2.mean + st2.stdev);
	d2.push_back(st2.mean - st2.stdev);
	data2.push_back(d2);
    }
    ostringstream title;
    title << "Date range: starts from sign-up date";
    create_csv(data, "summary_sum");
    create_plg_file("summary_sum", title.str(), "filtered", "unfiltered", html_stuff, "Overall users sum since sign-up");
    create_csv(data1, "summary_stdev1");
    create_plg_file("summary_stdev1", title.str(), "+1 stdev", "-1 stdev", html_stuff, "68% Bounds for filtered users since sign-up");
    create_csv(data2, "summary_stdev2");
    create_plg_file("summary_stdev2", title.str(), "+1 stdev", "-1 stdev", html_stuff, "68% Bounds for unfiltered users since sign-up");
}



void populate_name_date_hash(NameDateHash &hash, const vector<Rec> &recs, int gender)
{
    for(auto &r:recs) {
	if(gender == 1)
	    hash[r.name][r.date].count += r.count;
	else if(gender == 2)
	    hash[r.name][r.date].all_count += r.count;
    }
}

vector<string> read_list(const string &list_file)
{
    ifstream ifs(list_file);
    string line;
    vector<string> rgs;
    while(getline(ifs, line)) {
	istringstream iss(line);
	string rg;
	iss >> rg;
	if(!rg.empty() && rg[0] == '#') continue;
	rgs.emplace_back(rg);
    }
    ifs.close();
    return rgs;
}

void print_error_codes()
{
    cout<<"error_code======="<<endl;
    cout << "code="<< regex_constants::error_collate;
    cout << "code="<< regex_constants::error_ctype;	
    cout << "code="<< regex_constants::error_escape;	
    cout << "code="<< regex_constants::error_backref;	
    cout << "code="<< regex_constants::error_brack	;
    cout << "code="<< regex_constants::error_paren;	
    cout << "code="<< regex_constants::error_brace;	
    cout << "code="<< regex_constants::error_badbrace;	
    cout << "code="<< regex_constants::error_range;	
    cout << "code="<< regex_constants::error_space;	
    cout << "code="<< regex_constants::error_badrepeat;
    cout << "code="<< regex_constants::error_complexity;
    cout << "code="<< regex_constants::error_stack;
    cout<<"ENNNNNNNNNNNNNNNNND"<<endl;
}

vector<Rec> filter_in(const vector<Rec> &recs, const vector<string> &rgs, bool match_means_stay = true)
{
    //print_error_codes();
    vector<Rec> ans;
    for(auto &r:recs) {
	bool matched = false;
	auto err0 = r.url;
	string err2;
	try {
	    for(auto &rg:rgs) {
		err2 = rg;
		regex e(rg);
		if(regex_match(r.url, e)) {
		    matched = true;
		    break;
		}
	    }
	}
	catch (const std::regex_error& e) {
	    std::cout << "regex_error caught: " << e.what() << '\n';
	    cout <<" code = "<<e.code()<<endl;
	    if (e.code() == std::regex_constants::error_paren) {
		std::cout << "The code was error_paren\n";
		cout<<"url = "<<err0 << endl;
		cout<<"e = "<<err2 << endl;
	    }
	}
	if(match_means_stay) {
	    if(matched) ans.push_back(r);
	} else {
	    if(!matched) ans.push_back(r);
	}
    }
    return ans;
}

void populate_hash(DateNameHash &hash, const vector<Rec> &recs, int gender)
{
    for(auto &r:recs) {
	if(gender == 1)
	    hash[r.date][r.name].count += r.count;
	else if(gender == 2)
	    hash[r.date][r.name].all_count += r.count;
    }
}

vector<Rec> filter_out(const vector<Rec> &recs, const vector<string> &rgs)
{
    return filter_in(recs, rgs, false);
}

void print_urls(const vector<Rec> &recs, const string &output)
{
    ofstream ofs(output);
    for(auto &r:recs) {
	ofs<<r.url<<endl;
    }
    ofs.close();
}

void append_unique_urls(const vector<Rec> &recs, const string &output)
{
    ofstream ofs(output, fstream::app);
    unordered_set<string> us;
    for(auto &r:recs) {
	us.insert(r.url);
    }
    for(auto &s:us) {
	ofs<<s<<endl;
    }
    ofs.close();
}

void print_csv(const DateNameHash &hash, const map<string, int> &age, const string &output)
{
    ofstream ofs(output);
    ofs<<"year,age,sex,people"<<endl;
    for(auto &h:hash) {
	int date = h.first;
	auto &hash2 = h.second;
	for(auto &hh:hash2) {
	    auto name = hh.first;
	    auto found = age.find(name);
	    if(found != end(age)) {
		auto c = hh.second;
		ofs<<date<<","<<found->second<<","<<1<<","<<c.count<<endl;;
		ofs<<date<<","<<found->second<<","<<2<<","<<c.all_count<<endl;;
	    } else {
		cout<<"index mapping not found for "<<hh.first<<endl;
	    }
	}
    }
    ofs.close();
}

    template <class T>
void stream_take(istringstream& iss, T &t)
{
    string s;
    iss >> s;
    if(0 == s.compare("(not")) {
	iss >> s; //dummy
	s = "not_set";
    }
    istringstream iss2(s);
    iss2 >> t;
}

    template <class T>
void stream_take2(istringstream& iss, T &t)
{
    string dummy;
    iss >> dummy;
    stream_take(iss, t);
}

void load_ga_data(vector<Rec> &v, const string &file)
{
    ifstream ifs(file);
    string line;
    while(getline(ifs, line)) {
	istringstream iss(line);
	Rec rec;
	stream_take(iss, rec.name);
	stream_take(iss, rec.date);
	stream_take2(iss, rec.count);
	stream_take2(iss, rec.newUsers);
	stream_take2(iss, rec.sessions);
	stream_take2(iss, rec.bounceRate);
	stream_take2(iss, rec.pageviews);
	stream_take2(iss, rec.pageviewsPerSession);
	stream_take2(iss, rec.avgSessionDuration);
	stream_take2(iss, rec.url);
	stream_take2(iss, rec.medium);
	stream_take2(iss, rec.socialNetWork);
	v.push_back(rec);
	//verify it's loaded properly
	//if(0 == rec.url.compare("baylardlaw.com"))
	//cout <<"HEYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"<< rec.str()<<endl<<endl;
    }
    ifs.close();
}

void add_to_idx_map(map<string, int> &age, const vector<Rec> &recs)
{
    for(auto &r:recs) {
	auto found = age.find(r.name);
	if(found == end(age)) {
	    age[r.name] = age.size();
	}
    }
}

void print_idx_map(const map<string, int> &age, ostream &os)
{
    map<int, string> m;
    for(auto &p:age) {
	m.insert({p.second, p.first});
    }
    os<< "<body><p style=\"color:INDIANRED;\">";
    for(auto &p:m) {
	os<<p.first<<" "<<p.second<<"<br>"<<endl;
    }
    os<<"</p></body>";
}

vector<Rec> get_gray_recs(const vector<Rec> &original, const vector<string> &whitelist, const vector<string> &blacklist)
{
    auto no_whites = filter_out(original, whitelist);
    return filter_out(no_whites, blacklist);
} 

void populate_summary_hash(map<int, Count> &hash, const vector<Rec> &filtered_recs, const vector<Rec> &all_recs)
{
    for(auto &r:filtered_recs) {
	hash[r.date].count += r.count;
    }
    for(auto &r:all_recs) {
	hash[r.date].all_count += r.count;
    }
}

void create_plg_file(const string &filename_without_ext, const string &title, const string &legend1, const string &legend2, vector<pic_attr> &html_stuff, string big_title)
{
    ofstream ofs(filename_without_ext + ".plg");
    ofs << "set terminal pngcairo size 1440,800 linewidth 3 enhanced font 'Verdana,10'"<<endl;
    ofs<<"set output '"<<filename_without_ext<<".png'"<<endl;
    ofs<<"set title \""<<title<<"\""<<endl;
    ofs<<"plot '"<<filename_without_ext<<".csv' using 1:2 with lines title '"<<legend1<<"', '"<<filename_without_ext<<".csv' using 1:3 with lines title '"<<legend2<<"'"<<endl;
    ofs.close();
    string cmd{"gnuplot "};
    cmd += filename_without_ext;
    cmd += ".plg";
    system(cmd.c_str());
    pic_attr pa;
    pa.title = big_title;
    pa.name = filename_without_ext;
    html_stuff.push_back(pa);
}

void print_summary_hash(const map<int, Count> &hash, const string &filename_without_ext, map<string, int> &start_dates_map, vector<pic_attr> &html_stuff)
{
    if(hash.empty()) {
	cout<< "hash is empty!"<<endl;
	return;
    }
    string csv{filename_without_ext};
    ofstream ofs(csv+".csv");
    auto it = begin(hash);
    int start_date = start_dates_map[filename_without_ext];
    if(start_date) {
	it = hash.find(start_date);
	if(it == end(hash)) it = begin(hash);
    } else {
	start_date = it->first;
    }
    int i=0;
    while(it != end(hash)) {
	auto tmp = end(hash);
	auto count = it->second.count;
	auto all_count = it->second.all_count;
	auto ratio = get_projection_ratio();
	if(it == --tmp) {
	    count *= ratio;
	    all_count *= ratio;
	}
	ofs<<i++<<", "<<count<<", "<<all_count<<endl;
	++it;
    }
    int end_date = (--it)->first;
    ofs.close();
    ostringstream oss;
    oss << "Date range:"<< start_date<<" - "<< end_date;
    create_plg_file(filename_without_ext, oss.str(), "filtered", "unfiltered", html_stuff, "Overall user hits");
}

    template <class T>
void create_csv(const vector<vector<T>> &data, string filename)
{
    ofstream ofs(filename +".csv");
    int i = 0;
    if(data.empty()) {
	cout<<"data is empty for "<<filename<<endl;
    }
    for(auto &v:data) {
	ofs << i++;
	for(auto it = begin(v); it != end(v); ++it) {
	    ofs<<", "<<*it;
	}
	ofs<<endl;
    }
}

/*
   template <class T>
   void plot_two_lines(const vector<vector<T>> &data, const string &legend1, const string &legend2, const string &title, const string &filename_without_ext)
   {
   create_csv(data, filename_without_ext);
   create_plg_file(filename_without_ext, title, legend1, legend2);
   }
 */
void print_summary_ratio(const map<int, Count> &hash, const string &filename_without_ext, map<string, int> &start_dates_map,vector<pic_attr> &html_stuff)
{
    string csv{filename_without_ext};
    int i=0;
    int start_date = start_dates_map[filename_without_ext];
    int end_date =0;
    if(!hash.empty()) {
	auto it =  end(hash);
	end_date = (--it)->first;
    } else {
	cout << "summary ratio hash is empty"<<endl;
	return;
    }
    auto it = begin(hash);
    if(start_date) {
	it = hash.find(start_date);
    } else {
	start_date = it->first;
    }

    if(it == end(hash)) return;
    int prev_count = it->second.count;
    int prev_all_count = it->second.all_count;
    ++it;
    vector<vector<string>> data;
    while(it != end(hash)) {
	int new_count = it->second.count;
	int new_all_count = it->second.all_count;
	double count_ratio = new_count/(double)prev_count;
	double all_count_ratio = new_all_count/(double)prev_all_count;
	vector<string> vd{to_string(i++), to_string(count_ratio), to_string(all_count_ratio)};
	data.push_back(vd);
	prev_count = new_count;
	prev_all_count = new_all_count;
	++it;
    }
    ostringstream title;
    title << "Date range:"<< start_date<<" - "<< end_date;
    create_csv(data, filename_without_ext);
    create_plg_file(filename_without_ext, title.str(), "filtered", "unfiltered", html_stuff, "Normalized month-to-month users since sign-up");
}


void load_str_int_file(map<string, int> &m, const string &file, int divisor=1)
{
    ifstream ifs(file);
    if(!ifs.is_open()) {
	cout<<"FILE "<<file<<" NOT FOUND!"<<endl;
	exit(1);
    }

    string line;
    while(getline(ifs, line)) {
	string a;
	int d=0;
	istringstream iss(line);
	if(iss>>a && iss >>d) {
	    m[a] = d/divisor;
	}
    }
}

void load_start_dates(map<string, int> &start_dates_map, const string &dates_map_file)
{
    map<string, int> view_ids_map, add_to_dates_map;
    load_str_int_file(view_ids_map, "view_ids.txt");
    load_str_int_file(start_dates_map, dates_map_file, 100);

    ofstream ofs(dates_map_file, fstream::app);
    for(auto &p:view_ids_map) {
	if(!start_dates_map.count(p.first)) {
	    ofs<<p.first<<" "<<"20150101"<<endl;
	}
    }
    ofs.close();
}

void create_client_charts(const DateNameHash &hash, const map<string, int> &start_dates_map, const string &name, vector<pic_attr> &html_stuff)
{
    auto date_it = start_dates_map.find(name);
    if(date_it == end(start_dates_map)) return;
    int start_date = date_it->second;
    int end_date = 0;
    vector<vector<int>> data;
    auto it = hash.find(start_date);
    if(it == end(hash)) it = begin(hash);
    while(it != end(hash)) {
	auto name_it = it->second.find(name);
	if(name_it != end(it->second)) {
	    int count = name_it->second.count;
	    int all_count = name_it->second.all_count;
	    auto tmp = it;
	    auto ratio = get_projection_ratio();
	    if(++tmp == end(hash)) {
		count *= ratio;
		all_count *= ratio;
	    }
	    vector<int> v{count, all_count};
	    data.push_back(v);
	    end_date = it->first;
	}
	++it;
    }
    string legend{"everyone else except "};
    ostringstream title;
    title <<"Date range: "<< start_date<<" - "<<end_date;
    create_csv(data, name);
    ostringstream big_title;
    big_title<<name<<" users";
    create_plg_file(name, title.str(), "filtered", "unfiltered", html_stuff, big_title.str());
}

void create_all_client_charts(const DateNameHash &hash, const map<string, int> &start_dates_map, vector<pic_attr> &html_stuff)
{
    auto it = end(hash); --it;
    set<string> names;
    for(auto p:it->second)
	names.insert(p.first);

    for(auto &name:names) {
	create_client_charts(hash, start_dates_map, name, html_stuff);
    }
}

void kpi_from_vec(Kpi &kpi, const vector<Rec> &v, const vector<string> &white_list)
{
    auto filtered_recs = filter_in(v, white_list);
    int new_users{0};
    vector<string> paid_strings = {"cpa", "cpc", "cpm", "cpp", "cpv", "ppc"};
    auto is_paid = [&paid_strings](string s) {
	return find(begin(paid_strings), end(paid_strings), s) != end(paid_strings);
    };

    for(auto &r:filtered_recs) {
	int n = r.count;
	new_users += r.newUsers;
	kpi.pages_per_session += r.pageviewsPerSession * n;
	kpi.bounce_rate += r.bounceRate * n;
	kpi.avg_time_on_site += r.avgSessionDuration* n;
	if(r.date==201609 || r.date==201608) {
	    //cout << r.str()<<endl;
	    // cout << "page per session="<<kpi.pages_per_session<<endl;
	}
	if(0 == r.socialNetWork.compare("not_set")) {
	    string medium = r.medium;
	    if(0 == medium.compare("(none)")) {
		kpi.direct_users += n;
	    } else if(0 == medium.compare("referral")) {
		kpi.referral_users += n;
	    } else if(0 == medium.compare("organic")) {
		kpi.organic_search_users += n;
	    } else if(is_paid(medium)) {
		kpi.paid_users += n;
	    } else {
		kpi.other_users += n;
	    }
	} else {
	    kpi.social_users += n;
	}
    }
    int a = kpi.direct_users;
    int b = kpi.organic_search_users;
    int c = kpi.referral_users;
    int d = kpi.social_users;
    int e = kpi.paid_users;
    int f = kpi.other_users;
    kpi.total_users = a + b + c + d + e + f;
    double total_users = static_cast<double>(kpi.total_users);
    if(total_users == 0) total_users = 1; //avoid div by 0
    kpi.pages_per_session /= total_users;
    kpi.bounce_rate /= total_users;
    kpi.percent_new = 100* new_users/total_users;
    kpi.avg_time_on_site /= total_users;
}
void populat_kpi_map(KpiMap &kmap, const vector<Rec> &recs, const vector<string> &white_list)
{
    map<string, map<int, vector<Rec>>> mmv;
    for(auto &rec:recs) {
	mmv[rec.name][rec.date].push_back(rec);
    }

    for(auto &mv:mmv) {
	auto name = mv.first;
	for(auto &m:mv.second) {
	    auto date = m.first;
	    auto &v = m.second;
	    Kpi kpi;
	    kpi_from_vec(kpi, v, white_list);
	    kmap[name][date] = kpi;
	}
    }
}
/*
   struct Rec {
   string name;
   int date=0;
   int count=0;
   int newUsers;
   int sessions;
   double bounceRate;
   int pageviews;
   double pageviewsPerSession;
   double avgSessionDuration;
   string url;
   string socialNetWork;
   };

   struct Kpi {
   int total_users;
   double pages_per_session;
   double bounce_rate;
   double percent_new;
   double avg_time_on_site;
   int referral_users;
   int direct_users;
   int social_users;
   int organic_search_users;
   };
   using KpiMap = map<string, map<int, Kpi>>;
 */

void kpi_table_header(ostream &os)
{
    ostringstream html_junk;
    html_junk << "<head> <style> table { font-family: arial, sans-serif; border-collapse: collapse; width: 100%; }td, th {    border: 1px solid #dddddd;    text-align: left; padding: 8px; }\n";
    html_junk << " tr:nth-child(even) { background-color: #dddddd; } </style> </head> <body>\n"<<endl;
    string hello{"hello"};
    html_junk << "<table> <tr> <th>Date(YYYMM)</th> <th> Total New Users </th> <th>Pages Per Session</th> <th>Bounce Rate</th><th>% New Users</th><th>Avg Time on Site(seconds)</th><th>Referral Users</th><th>Direct Users</th><th>Social Users</th><th>Organic Search Users</th> </tr> \n";
    os<<html_junk.str();
}

void kpi_to_html(ostream &os, const KpiMap &kmap, const string &name)
{
    ostringstream html_junk;
    html_junk << "<head> <style> table { font-family: arial, sans-serif; border-collapse: collapse; width: 100%; }td, th {    border: 1px solid #dddddd;    text-align: left; padding: 8px; }\n";
    html_junk << " tr:nth-child(even) { background-color: #dddddd; } </style> </head> <body>\n"<<endl;
    string hello{"hello"};
    html_junk << "<table> <tr> <th>Date</th> <th> Total Users </th> <th>Pages Per Session</th> <th>Bounce Rate %</th><th>% New Users</th><th>Avg Time on Site(seconds)</th><th>Referral Users</th><th>Direct Users</th><th>Social Users</th><th>Organic Search Users</th><th>Paid Users</th><th>Other Users</th> </tr> \n";
    os<<html_junk.str();

    auto func = [&os](int date, const Kpi&k) {
	os<<"<tr>\n"
	    <<"<td>"<< date <<"</td>\n"
	    <<"<td>"<< k.total_users <<"</td>\n"
	    <<"<td>"<< k.pages_per_session <<"</td>\n"
	    <<"<td>"<< k.bounce_rate <<"</td>\n"
	    <<"<td>"<< k.percent_new <<"</td>\n"
	    <<"<td>"<< k.avg_time_on_site <<"</td>\n"
	    <<"<td>"<< k.referral_users <<"</td>\n"
	    <<"<td>"<< k.direct_users <<"</td>\n"
	    <<"<td>"<< k.social_users <<"</td>\n"
	    <<"<td>"<< k.organic_search_users<<"</td>\n"
	    <<"<td>"<< k.paid_users<<"</td>\n"
	    <<"<td>"<< k.other_users<<"</td>\n"
	    <<"</tr>\n";
    };
    auto found = kmap.find(name);
    if(found != end(kmap)) {
	for(auto &k:found->second) {
	    int date = k.first;
	    auto &kk = k.second;
	    func(date, kk);
	}
    }
    os<<"</table>";
}

void create_html(const vector<pic_attr> &v, const KpiMap &kmap, const KpiMap &kmap2,const NameDateUrlCount &mmm,const NameDateUrlCount &mmm2, const map<string, int> &age)
{
    ofstream ofs("index.html");
    if(!ofs.is_open()) {
	cout<<"index.html cannot be opened!"<<endl;
	return;
    }
    ofs << "<!DOCTYPE html> <meta charset=\"utf-8\"> <head> </head> <body> <hr>"<<endl;
    for(auto &p:v) {
	ofs<<" <p><font size=\"8\" color=\"purple\">"<<p.title<<"</font></p>"<<endl;
	ofs<<"<img src=\""<<p.name<<".png\"> \n";
	//kpi_table_header(ofs);
	ofs<<" <p><font size=\"5\" color=\"black\">Key Performance Indicator(no spam)</font></p>"<<endl;
	kpi_to_html(ofs, kmap, p.name);
	ofs<<" <p><font size=\"5\" color=\"black\">Key Performance Indicator(with spam)</font></p>"<<endl;
	kpi_to_html(ofs, kmap2, p.name);
	ofs<<"</table>";
	ofs<<" <p><font size=\"5\" color=\"black\">Top Referrals(no spam)</font></p>"<<endl;
	html_from_name_date_url_count(ofs, mmm, p.name);
	ofs<<" <p><font size=\"5\" color=\"black\">Top Referrals By Avg Time(no spam)</font></p>"<<endl;
	html_from_name_date_url_count(ofs, mmm2, p.name);
	ofs<<"<hr>"<<endl;
    }

    ofs<<"<p><font size=\"8\" color=\"black\">ALL Top Referrals(no spam)</font></p>"<<endl;
    html_from_name_date_url_count(ofs, mmm, "all");
    ofs<<" <p><font size=\"8\" color=\"purple\">"<<"Users Comparison Across All Firms"<<"</font></p>"<<endl;
    ifstream ifs("population.html");
    string line;
    while(getline(ifs, line)) {
	ofs << line <<endl;
    }
    ofs<<" <p><font size=\"5\" color=\"black\">Legend:</font></p>"<<endl;
    print_idx_map(age, ofs);
    ofs.close();
}



//IT'S HIGH NOON
int main(int argc, char *argv[])
{
    if(argc < 2) {
	cout <<"Usage: "<<argv[0] <<" <google analytics dataset(generated by .py)>"<<endl;
	exit(1);
    }
    map<string, int> start_dates_map;
    load_start_dates(start_dates_map, "start_dates.txt");
    string filename = argv[1];
    vector<string> white_list = read_list("whitelist.txt");
    vector<string> black_list = read_list("blacklist.txt");
    vector<Rec> recs;
    load_ga_data(recs, filename);

    { 
	cout<<"updating blacklist.txt"<<endl;
	auto gray_recs = get_gray_recs(recs, white_list, black_list);
	if(!gray_recs.empty()) {
	    append_unique_urls(gray_recs, "blacklist.txt");
	    cout<<"blacklist.txt updated.(new urls are appended)"<<endl; 
	} else {
	    cout<<"blacklist.txt already up-to-date, no change."<<endl; 
	}
    }
    map<string, int> age;
    add_to_idx_map(age, recs);
    auto filtered_recs = filter_in(recs, white_list);
    DateNameHash hash;
    populate_hash(hash, recs, 2);
    populate_hash(hash, filtered_recs, 1);
    NameDateHash name_date_hash;

    populate_name_date_hash(name_date_hash, recs, 2);
    populate_name_date_hash(name_date_hash, filtered_recs, 1);
    print_csv(hash, age, "populationz.csv");

    map<int, Count> summary_hash;
    populate_summary_hash(summary_hash, filtered_recs, recs);
    vector<pic_attr> html_stuff;
    print_summary_hash(summary_hash, "summary_plot", start_dates_map, html_stuff);
    //print_summary_ratio(summary_hash, "summary_ratio_plot", start_dates_map, html_stuff);
    do_some_stats(name_date_hash, start_dates_map, html_stuff);
    create_all_client_charts(hash, start_dates_map, html_stuff);

    KpiMap kpi_everyone, kpi_everyone_unfiltered;
    cout<<"populating tables...\n";
    populat_kpi_map(kpi_everyone, recs, white_list);
    vector<string> tmp = {".*"};
    populat_kpi_map(kpi_everyone_unfiltered, recs, tmp);
    NameDateUrlCount mmm, mmm2;
    populate_name_date_url_count(mmm, filtered_recs);
    populate_name_date_url_avg_time(mmm2, filtered_recs);
    cout<<"done populating tables\n";
    create_html(html_stuff, kpi_everyone, kpi_everyone_unfiltered, mmm, mmm2 ,age);
}
