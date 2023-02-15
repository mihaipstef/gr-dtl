from db import db
from models import (
    MonitorData,
    Subscribers,
)


def create_subscibers(pair_id: str, rx_sub_id: str, tx_sub_id: str, **kwargs):
    subs = Subscribers()
    try:
        subs.pair_id = pair_id
        subs.rx_sub_id = rx_sub_id
        subs.tx_sub_id = tx_sub_id
        subs.__dict__.update(kwargs)
    except:
        return None
    db.session.add(subs)
    db.session.commit()
    return subs


def retrieve_subscribers(is_active: bool = None):
    q = db.session.query(
        Subscribers,
    )
    if is_active is not None:
        q.filter(
            Subscribers.is_active == is_active,
        )
    q.order_by(Subscribers.date_updated.desc())
    return q.all()


def retrieve_subscriber(pair_id):
    return db.session.query(
        Subscribers,
    ).filter(
        Subscribers.pair_id == pair_id
    ).one_or_none()


def update_subscribers(pair_id: str, **kwargs):
    try:
        sub = retrieve_subscribers(True, pair_id)[0]
    except:
        return None
    sub.update(**kwargs)
    return sub


def retrieve_monitor_data(pair_id: str):
    s = db.session.query(
        MonitorData,
    ).filter(
        MonitorData.pair_id == pair_id,
    ).one_or_none()
    return s


def create_monitor_data(pair_id: str, **kwargs):
    s = MonitorData()
    try:
        s.pair_id = pair_id
        s.update(**kwargs)
    except Exception as e:
        print(str(e))
        return None
    db.session.add(s)
    db.session.commit()
    return s


def update_monitor_data(pair_id: str, **kwargs):
    s = retrieve_monitor_data(pair_id)
    if s:
        try:
            s.update(**kwargs)
        except:
            return None
        db.session.add(s)
        db.session.commit()
    return s


def delete_sattistics(pair_id: str):
    s = retrieve_monitor_data(pair_id)
    if s:
        db.session.delete(s)
        db.session.commit()
