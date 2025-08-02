from flask import Flask, jsonify, request
import requests
import xmltodict
import schedule
import time
from threading import Thread

app = Flask(__name__)

EPG_SOURCES = [
    {"name": "source1", "url": "http://example.com/source1.xml"},
    {"name": "source2", "url": "http://example.com/source2.xml"}
]

@app.route('/health', methods=['GET'])
def health():
    return jsonify({"status": "healthy"}), 200

@app.route('/epg', methods=['GET'])
def get_epg():
    epg_data = []
    for source in EPG_SOURCES:
        try:
            response = requests.get(source['url'])
            if response.status_code == 200:
                data = xmltodict.parse(response.content)
                epg_data.append({"source": source['name'], "data": data})
        except Exception as e:
            print(f"Error fetching data from {source['name']}: {e}")
    return jsonify(epg_data)

def update_epg():
    print("Updating EPG data...")
    # Logic to update EPG data

schedule.every(30).minutes.do(update_epg)

def run_schedule():
    while True:
        schedule.run_pending()
        time.sleep(1)

if __name__ == '__main__':
    Thread(target=run_schedule).start()
    app.run(host='0.0.0.0', port=5002)
