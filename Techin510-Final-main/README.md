# Personal Finance Advisor 🏦
https://sp14-techin510-lab6-5vgjbpcb85zmcdbajeaanf.streamlit.app/
## Getting Started

```
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt

streamlit run app.py

```

## Lessons Learned
- if `streamlit run app.py` cannot work try `python -m streamlit run app.py`
- Transit .csv to visualize diagram(column diagram & scatter plot)
- Using `sidebar` and `expander`

## Questions
- Hard to connect to Supabase database
```
psycopg2.OperationalError: connection to server on socket "/tmp/.s.PGSQL.5432" failed: FATAL:  database "junyanlu" does not exist
```

## TODO
- Solve problems which relate SQL
