import boto3

# 从环境变量中读取AWS凭证和区域
aws_access_key_id = "AKIAXYKJUDVTMEUFYIHC"
aws_secret_access_key = "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"
aws_region = "us-east-2"

# 创建DynamoDB客户端
dynamodb = boto3.resource('dynamodb', 
                          region_name=aws_region,
                          aws_access_key_id=aws_access_key_id,
                          aws_secret_access_key=aws_secret_access_key)

# 指定表名
table_name = 'SensorData_v1'
table = dynamodb.Table(table_name)

# 扫描表以获取所有项
response = table.scan()
items = response['Items']

# 打印更新后的表项
for item in items:
    print(item)
