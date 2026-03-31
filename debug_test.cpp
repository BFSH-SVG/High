#include <iostream>
#include <string>
#include <unordered_map>
using namespace std;

class Solution {
public:
    int lengthOfLongestSubstring(string s) {
        unordered_map<char,int> hash;
        int n = s.size();
        int l = 0;  // ⬅️ 调试点1：初始化 l
        int maxLength = 0;
        
        cout << "输入字符串: " << s << endl;
        
        for(int r = 0; r < n; r++) {
            char c = s[r];
            cout << "r=" << r << ", c='" << c << "'";
            
            if(hash.find(c) != hash.end() && hash[c] >= l) {  // ⬅️ 调试点2：打印判断条件
                cout << " -> 重复! hash[" << c << "]=" << hash[c];
                l = hash[c] + 1;
                cout << ", 新l=" << l;
            }
            
            hash[c] = r;
            cout << ", l=" << l << ", r=" << r;
            
            maxLength = max(maxLength, r - l + 1);
            cout << ", 当前长度=" << r - l + 1 << ", maxLength=" << maxLength << endl;
        }
        return maxLength;
    }
};

int main() {
    Solution sol;
    
    cout << "===== 测试1 =====" << endl;
    cout << "结果: " << sol.lengthOfLongestSubstring("abcabcbb") << endl;
    
    cout << "\n===== 测试2 =====" << endl;
    cout << "结果: " << sol.lengthOfLongestSubstring("abba") << endl;
    
    return 0;
}
