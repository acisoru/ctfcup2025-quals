from flask import Flask, session, request

app = Flask(__name__)
app.config["SECRET_KEY"] = "kluchik"


@app.route("/", methods=["POST"])
def index():
    session.update(request.json)
    return "a"


app.run()
