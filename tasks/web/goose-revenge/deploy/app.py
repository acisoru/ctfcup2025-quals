#!/usr/bin/env python3

from flask import Flask, request, jsonify, session, render_template
import os
from flask_pymongo import PyMongo, ObjectId
from werkzeug.security import generate_password_hash, check_password_hash
from bson import ObjectId
import secrets
import json

app = Flask(__name__)
app.config["SECRET_KEY"] = "kluchik"
app.config["MONGO_URI"] = (
    "mongodb://mongo111:mongo111@mongo:27017/mongo111?authSource=admin"
)

# Initialize MongoDB
mongo = PyMongo(app)


def init_flag():
    mongo.db.strings.update_one(
        {
            "name": "flag",
        },
        {
            "$set": {
                "name": "flag",
                "user_id": secrets.token_hex(32),
                "content": os.getenv("FLAG"),
            },
        },
        upsert=True,
    )


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/register", methods=["POST"])
def register():
    data = request.get_json()

    if not data or not data.get("username") or not data.get("password"):
        return jsonify({"error": "Username and password required"}), 400

    if mongo.db.users.find_one({"username": str(data["username"])}):
        return jsonify({"error": "Username already exists"}), 400

    hashed_password = generate_password_hash(data["password"])

    user_id = mongo.db.users.insert_one(
        {"username": str(data["username"]), "password": hashed_password}
    ).inserted_id

    return (
        jsonify({"message": "User created successfully", "user_id": str(user_id)}),
        201,
    )


@app.route("/login", methods=["POST"])
def login():
    data = request.get_json()

    if not data or not data.get("username") or not data.get("password"):
        return jsonify({"error": "Username and password required"}), 400

    user = mongo.db.users.find_one({"username": str(data["username"])})

    if user and check_password_hash(user["password"], data["password"]):
        session["user_id"] = str(user["_id"])
        return jsonify({"message": "Login successful"}), 200

    return jsonify({"error": "Invalid credentials"}), 401


@app.route("/logout", methods=["POST"])
def logout():
    session.clear()
    return jsonify({"message": "Logged out successfully"}), 200


@app.route("/strings", methods=["POST"])
def create_string():
    if not session:
        return jsonify({"error": "Please login first"}), 401

    data = request.get_json()

    if not data or not data.get("name") or not data.get("content"):
        return jsonify({"error": "Name and content required"}), 400

    if data["name"] == "flag":
        return jsonify({"error": "Cannot overwrite flag"}), 400

    # Check if string name already exists for this user
    existing_string = mongo.db.strings.find_one(
        {"user_id": str(session["user_id"]), "name": str(data["name"])}
    )

    if existing_string:
        return jsonify({"error": "String with this name already exists"}), 400

    string_id = mongo.db.strings.insert_one(
        {
            "user_id": str(session["user_id"]),
            "name": str(data["name"]),
            "content": str(data["content"]),
        }
    ).inserted_id

    return (
        jsonify(
            {"message": "String created successfully", "string_id": str(string_id)}
        ),
        201,
    )


@app.route("/strings/<string:name>", methods=["GET"])
def get_string(name):
    if not session:
        return jsonify({"error": "Please login first"}), 401

    string_data = mongo.db.strings.find_one(
        {"user_id": str(session["user_id"]), "name": name}
    )

    if not string_data:
        return jsonify({"error": "String not found"}), 404

    return (
        jsonify({"name": string_data["name"], "content": string_data["content"]}),
        200,
    )


@app.route("/strings", methods=["GET"])
def get_all_strings():
    if not session:
        return jsonify({"error": "Please login first"}), 401

    user_strings = list(mongo.db.strings.find(dict(session.items())))

    strings_list = []

    for string_data in user_strings:
        if string_data["user_id"] != session.get("user_id"):
            return jsonify({"error": "ooops, some string is not you own"}), 401
        strings_list.append(
            {"name": string_data["name"], "content": string_data["content"]}
        )

    return jsonify({"strings": strings_list}), 200


@app.route("/profile", methods=["GET"])
def profile():
    if not session:
        return jsonify({"error": "Please login first"}), 401

    return (
        jsonify(
            {
                "user_id": session["user_id"],
                "username": mongo.db.users.find_one(
                    {"_id": ObjectId(session["user_id"])}
                )["username"],
            }
        ),
        200,
    )


if __name__ == "__main__":
    init_flag()
    app.run("0.0.0.0", port=9119)
