import pytest

from testsuite.databases import pgsql


# Start the tests via `make test-debug` or `make test-release`

# tests/test_basic.py
async def test_first_time_users(service_client):
    response = await service_client.post(
        '/v1/hello',
        params={'name': 'userver'},
    )
    assert response.status == 200
    assert response.text == 'Hello, userver!\n'

async def test_db_updates(service_client):
    response = await service_client.post('/v1/hello', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hello, World!\n'

    response = await service_client.post('/v1/hello', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'

    response = await service_client.post('/v1/hello', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'

    response = await service_client.get('/v1/top10')
    assert response.status == 200
    assert response.text == 'World 3\n'


@pytest.mark.pgsql('db-1', files=['initial_data.sql'])
async def test_db_initial_data(service_client):
    response = await service_client.post(
        '/v1/hello',
        params={'name': 'user-from-initial_data.sql'},
    )
    assert response.status == 200
    assert response.text == 'Hi again, user-from-initial_data.sql!\n'

async def test_top10(service_client):
    response = await service_client.get('/v1/top10')
    assert response.status == 200
    assert response.text == ''
    
    for n in ['world', 'earth', 'universe', 'userver']:
        response = await service_client.post('/v1/hello', params={'name': n})
        assert response.status == 200
        assert response.text == f'Hello, {n}!\n'

    response = await service_client.post('/v1/hello', params={'name': 'userver'})
    assert response.status == 200

    response = await service_client.get('/v1/top10')
    assert response.status == 200
    assert response.text == 'userver 2\nearth 1\nuniverse 1\nworld 1\n'

