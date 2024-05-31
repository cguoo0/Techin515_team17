import streamlit as st
import boto3
import os
from dotenv import load_dotenv
from datetime import datetime
import pandas as pd
import matplotlib.pyplot as plt  # 添加这个导入语句

# 加载环境变量
load_dotenv()

# 从环境变量中读取AWS凭证和区域
aws_access_key_id = os.getenv("AWS_ACCESS_KEY_ID")
aws_secret_access_key = os.getenv("AWS_SECRET_ACCESS_KEY")
aws_region = os.getenv("AWS_REGION")

# 打印环境变量以验证是否正确加载
print(f"AWS_ACCESS_KEY_ID: {aws_access_key_id}")
print(f"AWS_SECRET_ACCESS_KEY: {aws_secret_access_key}")
print(f"AWS_REGION: {aws_region}")

# 创建DynamoDB客户端
dynamodb = boto3.resource('dynamodb', 
                          region_name=aws_region,
                          aws_access_key_id=aws_access_key_id,
                          aws_secret_access_key=aws_secret_access_key)

# 从DynamoDB获取数据的函数
def fetch_weight_data(sensor_id):
    table = dynamodb.Table('SensorData_v1')
    response = table.query(
        KeyConditionExpression=boto3.dynamodb.conditions.Key('Sensor').eq(sensor_id)
    )
    data = response['Items']
    return data

# 从DynamoDB数据创建Pandas DataFrame的函数
def create_dataframe(data):
    df = pd.DataFrame(data)
    df['Timestamp'] = pd.to_datetime(df['Timestamp'], format='%m/%d/%Y, %H:%M')
    df['Weight'] = df['Weight'].apply(lambda x: float(x.split()[0]))  # Assuming Weight is in 'g'
    df = df[df['Weight'] >= 0]  # 过滤掉负数的 Weight 值
    df = df.sort_values(by='Timestamp')
    return df

# 绘制单个日期折线图的函数
def plot_daily_chart(df, date):
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(df['Timestamp'], df['Weight'], marker='o', label='Weight')
    
    # 填充折线图下方区域
    ax.fill_between(df['Timestamp'], df['Weight'], color='lightblue', alpha=0.5)
    
    # 找到最高点
    max_idx = df['Weight'].idxmax()
    max_time = df.loc[max_idx, 'Timestamp']
    max_weight = df.loc[max_idx, 'Weight']
    
    # 绘制最高点为红点
    ax.plot(max_time, max_weight, marker='o', color='red', markersize=10, label='Max Weight')
    
    ax.set_xlabel('Time')
    ax.set_ylabel('Weight (g)')
    ax.set_title(f'Pet Weight on {date.strftime("%Y-%m-%d")}')
    ax.tick_params(axis='x', rotation=45)
    ax.legend()
    st.pyplot(fig)
    plt.close(fig)

# 主应用程序
def main():
    st.title("Pet Weight Management")
    pages = ["Pet Daily Diagrams", "Pet Weight Management", "Feeding Plan"]
    page = st.sidebar.selectbox("Select a page", pages)

    if page == "Pet Daily Diagrams":
        st.header("Pet Daily Diagrams")
        sensor_id = st.text_input("Enter Sensor ID")
        if st.button("Fetch Data"):
            with st.spinner("Fetching data from DynamoDB..."):
                data = fetch_weight_data(sensor_id)
                if not data:
                    st.error("No data found for the given Sensor ID.")
                else:
                    df = create_dataframe(data)
                    st.success("Data fetched successfully!")
                    st.write(df)

                    # 根据日期分组并绘制每个日期的图表
                    df['Date'] = df['Timestamp'].dt.date
                    for date, group in df.groupby('Date'):
                        st.subheader(f"Weight Data for {date}")
                        plot_daily_chart(group, date)

    elif page == "Pet Weight Management":
        st.header("Pet Weight Management")
        pet_type = st.selectbox("Select pet type", ["Dog", "Cat", "Others"])

        if pet_type == "Dog":
            dog_breeds = ["Labrador Retriever", "German Shepherd", "Golden Retriever", "Bulldog", "Beagle",
                          "Poodle", "Rottweiler", "Boxer", "Siberian Husky", "Chihuahua"]
            pet_breed = st.selectbox("Select dog breed", dog_breeds)
        elif pet_type == "Cat":
            cat_breeds = ["Ragdoll", "Siamese", "Birman", "Persian", "Maine Coon",
                          "British Shorthair", "Sphynx", "Abyssinian", "Bengal", "Russian Blue"]
            pet_breed = st.selectbox("Select cat breed", cat_breeds)
        else:
            pet_breed = st.text_input("Enter pet breed")

        current_weight = st.number_input("Enter current weight (kg)")
        expected_weight = st.number_input("Enter expected weight (kg)")

    elif page == "Feeding Plan":
        st.header("Feeding Plan")
        pet_name = st.text_input("Enter pet name")
        num_meals = st.number_input("Number of meals per day", min_value=1, value=3, step=1)

        feeding_plan = {}
        for i in range(int(num_meals)):
            meal_name = f"Meal {i+1}"
            quantity = st.number_input(f"{meal_name} quantity (grams)", min_value=0, value=0, step=1)
            release_time = st.time_input(f"{meal_name} release time")
            feeding_plan[meal_name] = {"quantity": quantity, "release_time": str(release_time)}

        if st.button("Save Feeding Plan"):
            st.success("Feeding plan saved successfully!")

if __name__ == "__main__":
    main()



