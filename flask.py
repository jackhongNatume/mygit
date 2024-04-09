from flask import Flask, render_template, request, redirect, url_for
import datetime

app = Flask(__name__)

# 储存提醒事项的列表
reminders = []

@app.route('/')
def index():
    return render_template('index.html', reminders=reminders)

@app.route('/add_reminder', methods=['POST'])
def add_reminder():
    reminder_time = request.form['time']
    reminder_content = request.form['content']

    # 将时间字符串转换为时间对象
    reminder_time = datetime.datetime.strptime(reminder_time, '%Y-%m-%dT%H:%M')

    # 添加提醒事项到列表
    reminders.append({'time': reminder_time, 'content': reminder_content})

    # 重定向到首页
    return redirect(url_for('index'))

if __name__ == '__main__':
    app.run(debug=True)
