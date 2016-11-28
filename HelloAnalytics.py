#!/usr/bin/python
import sys, getopt
import datetime
import pprint
from time import sleep

"""Hello Analytics Reporting API V4."""

import argparse

from apiclient.discovery import build
import httplib2
from oauth2client import client
from oauth2client import file
from oauth2client import tools

SCOPES = ['https://www.googleapis.com/auth/analytics.readonly']
DISCOVERY_URI = ('https://analyticsreporting.googleapis.com/$discovery/rest')
CLIENT_SECRETS_PATH = 'client_secrets_other.json' # Path to client_secrets.json file.
#VIEW_ID = '111296951'
VIEW_IDS = {}
MONTH_START = ["01-01", "02-01", "03-01","04-01", "05-01", "06-01","07-01", "08-01", "09-01", "10-01", "11-01", "12-01"]
MONTH = ["01", "02", "03","04", "05", "06","07", "08", "09", "10", "11", "12"]
MONTH_END = ["01-31", "02-28", "03-31","04-30", "05-31", "06-30","07-31", "08-31", "09-30", "10-30", "11-30", "12-30"]

def load_view_ids():
    view_id_file = open('view_ids.txt', 'r')
    for line in view_id_file:
	v = line.split()
	VIEW_IDS[v[0]] = v[1]

def initialize_analyticsreporting():
  """Initializes the analyticsreporting service object.

  Returns:
    analytics an authorized analyticsreporting service object.
  """
  # Parse command-line arguments.
  parser = argparse.ArgumentParser(
      formatter_class=argparse.RawDescriptionHelpFormatter,
      parents=[tools.argparser])
  flags = parser.parse_args([])

  # Set up a Flow object to be used if we need to authenticate.
  flow = client.flow_from_clientsecrets(
      CLIENT_SECRETS_PATH, scope=SCOPES,
      message=tools.message_if_missing(CLIENT_SECRETS_PATH))

  # Prepare credentials, and authorize HTTP object with them.
  # If the credentials don't exist or are invalid run through the native client
  # flow. The Storage object will ensure that if successful the good
  # credentials will get written back to a file.
  storage = file.Storage('analyticsreporting.dat')
  credentials = storage.get()
  if credentials is None or credentials.invalid:
    credentials = tools.run_flow(flow, storage, flags)
  http = credentials.authorize(http=httplib2.Http())

  # Build the service object.
  analytics = build('analytics', 'v4', http=http, discoveryServiceUrl=DISCOVERY_URI)

  return analytics

def get_report(analytics, view_id, date):
  idx = int(date[-2:])-1
  year = date[:4]
  date0 = year+'-'+MONTH_START[idx]
  date1 = year+'-'+MONTH_END[idx]
  print("date0="+date0 +" date1="+date1)
  # Use the Analytics Service Object to query the Analytics Reporting API V4.
  return analytics.reports().batchGet(
      body={
        'reportRequests': [
        {
          'viewId': view_id,
          'dateRanges': [{'startDate': date0, 'endDate': date1}],
          'metrics': [
	  {'expression': 'ga:users'}, 
	  {'expression': 'ga:newUsers'}, 
	  {'expression': 'ga:sessions'}, 
	  {'expression': 'ga:bounceRate'}, 
	  #{'expression':'ga:organicSearches'},
	  {'expression': 'ga:pageviews'}, 
	  {'expression': 'ga:pageviewsPerSession'}, 
	  {'expression': 'ga:avgSessionDuration'}, 
	  ], 
	  'dimensions' : [
	  {'name':'ga:source'}, 
	  {'name':'ga:medium'},
	  {'name':'ga:socialNetwork'},
	  #{'name': 'ga:fullReferrer'}, 
	  #{'name': 'ga:userType'}, 
	  #{'name': 'ga:sessionCount'}, 
	  ]
        }]
      }
  ).execute()

def print_response(response):
  """Parses and prints the Analytics Reporting API V4 response"""
  for report in response.get('reports', []):
    columnHeader = report.get('columnHeader', {})
    dimensionHeaders = columnHeader.get('dimensions', [])
    metricHeaders = columnHeader.get('metricHeader', {}).get('metricHeaderEntries', [])
    rows = report.get('data', {}).get('rows', [])

    for row in rows:
      dimensions = row.get('dimensions', [])
      dateRangeValues = row.get('metrics', [])

      for header, dimension in zip(dimensionHeaders, dimensions):
        print(header + ': ' + dimension)

      for i, values in enumerate(dateRangeValues):
        print('Date range (' + str(i) + ')')
        for metricHeader, value in zip(metricHeaders, values.get('values')):
	    print(metricHeader.get('name') + ': ' + value)

def save_response(response, view_id, date_tag, file):
  have_data = False
  for report in response.get('reports', []):
    columnHeader = report.get('columnHeader', {})
    dimensionHeaders = columnHeader.get('dimensions', [])
    metricHeaders = columnHeader.get('metricHeader', {}).get('metricHeaderEntries', [])
    rows = report.get('data', {}).get('rows', [])

    for row in rows:
      dimensions = row.get('dimensions', [])
      dateRangeValues = row.get('metrics', [])
      file.write(view_id + " " + date_tag)
      have_data = True

      for i, values in enumerate(dateRangeValues):
        for metricHeader, value in zip(metricHeaders, values.get('values')):
          file.write(" "+metricHeader.get('name') + ': ' + value),

      for header, dimension in zip(dimensionHeaders, dimensions):
        file.write(' '+str(header) + ': '),
	if(all(ord(c)<128 for c in dimension)):
	    file.write(dimension),
	else:
	    file.write("non-ascii-link"),
      file.write(" END\n")
  return have_data

def main():
  date_list= [];
  dummy = [];
  if(len(sys.argv) == 2):
      date_list.append(sys.argv[1])
  if not date_list :
      today = datetime.date.today()
      for yr in range(2015, today.year+1):
	  rg = range(1, 13)
	  #print("yr="+str(yr))
	  if yr == today.year:
	      rg = range(1, today.month+1)
	  for r in rg:
	      #print("month="+str(r))
	      date = str(yr)+MONTH[r-1]
	      date_list.append(date)
  load_view_ids()
  pprint.pprint(VIEW_IDS)
  analytics = initialize_analyticsreporting()
  for date in date_list:
      file = open('output_by_month.'+date+'.txt', 'w')
      for firm in VIEW_IDS:
	  firm_str = firm+" "+ date
	  try:
	      print("Getting data for: "+ firm_str) 
	      response = get_report(analytics, str(VIEW_IDS[firm]), date)
	      have_data = save_response(response, str(firm), date, file)
	      sleep(1) #THROTTLE
	      if(have_data):
		  print("Received data for: "+ firm_str) 
	  except:
	      line = firm + " EXCEPTION: " + date + " "
	      print(line + str(sys.exc_info()[0]))

      file.close()

if __name__ == '__main__':
    main()
