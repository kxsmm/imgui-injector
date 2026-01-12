start_time=$(date +%s)
ndk-build
if [ $? -ne 0 ]; then
    echo "构建失败！"
    exit 1
fi
end_time=$(date +%s)
time_cost=$((end_time - start_time))
minutes=$((time_cost / 60))
seconds=$((time_cost % 60))
echo "--------------------------------"
echo "构建完成！"
echo "总耗时: ${time_cost} 秒"
echo "约等于: ${minutes} 分 ${seconds} 秒"
echo "--------------------------------"